// port64_ast: one clang pass over one src/ TU for the 64-bit worklist (tools/port/port64_worklist.py).
//
//   port64_ast --root ROOT --mode 64|32 --out OUT.json SOURCE -- <clang driver args>
//
// Writes one JSON object:
//   files    every file the TU read (the driver's cache key)
//   records  every complete, non-dependent record defined under ROOT: size, align, bases, fields (offset, size, type,
//            category, element record), vtable slots (MSVC-rule index -> Itanium index) for polymorphic ones
//   sites    (mode 64 only) expressions whose types lie about the object at 64 bits: raw byte offsets off a record
//            pointer, records reinterpreted as other records, field addresses reinterpreted, scalar views indexed,
//            pointer <-> integer casts with the member / parameter that holds the integer, constant-size allocations
//            and mem* calls, calls through computed function pointers (vtable slot numbers)
//   diags    warnings (mode 64: the -W flags the driver passes) and errors (mode 32: failed MB_LAYOUT32_ASSERTs)
// Nothing is compiled to code (-fsyntax-only).
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/VTableBuilder.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>

using namespace clang;
namespace json = llvm::json;

static llvm::cl::opt<std::string> OptRoot("root", llvm::cl::Required);
static llvm::cl::opt<int> OptMode("mode", llvm::cl::init(64));
static llvm::cl::opt<std::string> OptOut("out", llvm::cl::Required);
static llvm::cl::list<std::string> OptSrc(llvm::cl::Positional, llvm::cl::OneOrMore);

static json::Array g_records, g_sites, g_diags;
static std::set<std::string> g_files;

static std::string rel(StringRef p) {
  SmallString<256> buf(p);
  llvm::sys::path::remove_dots(buf, true);
  std::string s = std::string(buf);
  if (s.rfind(OptRoot, 0) == 0) {
    s = s.substr(OptRoot.size());
    if (!s.empty() && s[0] == '/') s = s.substr(1);
    return s;
  }
  return "";
}

class Diags : public DiagnosticConsumer {
 public:
  void HandleDiagnostic(DiagnosticsEngine::Level lvl, const Diagnostic &info) override {
    DiagnosticConsumer::HandleDiagnostic(lvl, info);
    if (lvl < DiagnosticsEngine::Warning || !info.hasSourceManager() || !info.getLocation().isValid()) return;
    const SourceManager &sm = info.getSourceManager();
    SourceLocation fl = sm.getFileLoc(info.getLocation());
    PresumedLoc pl = sm.getPresumedLoc(fl);
    if (pl.isInvalid()) return;
    std::string f = rel(pl.getFilename());
    if (f.empty()) return;
    SmallString<256> msg;
    info.FormatDiagnostic(msg);
    StringRef flag = info.getDiags()->getDiagnosticIDs()->getWarningOptionForDiag(info.getID());
    std::string macro;
    if (info.getLocation().isMacroID())
      macro = Lexer::getImmediateMacroNameForDiagnostics(info.getLocation(), sm, LangOptions()).str();
    g_diags.push_back(json::Object{{"file", f}, {"line", pl.getLine()}, {"col", pl.getColumn()},
                                   {"level", lvl >= DiagnosticsEngine::Error ? "error" : "warning"},
                                   {"flag", flag.str()}, {"msg", std::string(msg)}, {"macro", macro}});
  }
};

struct Ctx {
  ASTContext *ac;
  SourceManager *sm;
  PrintingPolicy pp;
};

static std::string tname(Ctx &c, QualType t) {
  return t.getUnqualifiedType().getAsString(c.pp);
}

// record key: the qualified type name; anonymous records by location
static std::string rkey(Ctx &c, const RecordDecl *rd) {
  if (!rd) return "";
  if (!rd->getIdentifier() && !rd->getTypedefNameForAnonDecl()) {
    PresumedLoc pl = c.sm->getPresumedLoc(c.sm->getFileLoc(rd->getLocation()));
    if (pl.isInvalid()) return "anon@?";
    return "anon@" + rel(pl.getFilename()) + ":" + std::to_string(pl.getLine()) + ":" + std::to_string(pl.getColumn());
  }
  return tname(c, c.ac->getCanonicalTagType(rd));
}

// P pointer / reference / function / member pointer, I integer / enum / bool, F floating, R record, ? other
static char cat(QualType t) {
  t = t.getCanonicalType();
  if (t->isPointerType() || t->isReferenceType() || t->isMemberPointerType() || t->isBlockPointerType() ||
      t->isFunctionType() || t->isNullPtrType())
    return 'P';
  if (t->isIntegralOrEnumerationType()) return 'I';
  if (t->isRealFloatingType()) return 'F';
  if (t->isRecordType()) return 'R';
  if (t->isVectorType()) return 'V';
  return '?';
}

static bool hasPtr(Ctx &c, QualType t, int depth = 0) {
  t = t.getCanonicalType();
  if (depth > 40) return false;
  while (const ArrayType *at = c.ac->getAsArrayType(t)) t = at->getElementType().getCanonicalType();
  if (cat(t) == 'P') return true;
  if (const RecordType *rt = t->getAs<RecordType>()) {
    const RecordDecl *rd = rt->getDecl()->getDefinition();
    if (!rd) return false;
    if (auto *cx = dyn_cast<CXXRecordDecl>(rd)) {
      if (cx->isDynamicClass()) return true;
      for (auto &b : cx->bases())
        if (hasPtr(c, b.getType(), depth + 1)) return true;
    }
    for (const FieldDecl *f : rd->fields())
      if (hasPtr(c, f->getType(), depth + 1)) return true;
  }
  return false;
}

static std::string locf(Ctx &c, SourceLocation l, unsigned *line, unsigned *col) {
  PresumedLoc pl = c.sm->getPresumedLoc(c.sm->getFileLoc(l));
  if (pl.isInvalid()) return "";
  *line = pl.getLine();
  *col = pl.getColumn();
  return rel(pl.getFilename());
}

static void emitRecord(Ctx &c, const RecordDecl *rd) {
  if (!rd->isCompleteDefinition() || rd->isInvalidDecl() || rd->isDependentType()) return;
  if (auto *cx = dyn_cast<CXXRecordDecl>(rd))
    if (cx->isLambda()) return;
  unsigned line = 0, col = 0;
  std::string f = locf(c, rd->getLocation(), &line, &col);
  if (f.empty()) return;
  const ASTRecordLayout &L = c.ac->getASTRecordLayout(rd);
  json::Object o{{"key", rkey(c, rd)}, {"file", f}, {"line", line},
                 {"size", L.getSize().getQuantity()}, {"align", L.getAlignment().getQuantity()},
                 {"union", rd->isUnion()}, {"hasptr", hasPtr(c, c.ac->getCanonicalTagType(rd))}};
  json::Array bases, fields, slots;
  auto *cx = dyn_cast<CXXRecordDecl>(rd);
  if (cx) {
    o["poly"] = cx->isPolymorphic();
    o["regs"] = cx->canPassInRegisters();     // passed by value in registers (trivial for calls) or by address
    o["trivcopy"] = cx->hasTrivialCopyConstructor();
    o["ownvptr"] = L.hasOwnVFPtr();
    for (auto &b : cx->bases()) {
      const CXXRecordDecl *bd = b.getType()->getAsCXXRecordDecl();
      if (!bd || !bd->hasDefinition()) continue;
      bd = bd->getDefinition();
      int64_t off = b.isVirtual() ? L.getVBaseClassOffset(bd).getQuantity() : L.getBaseClassOffset(bd).getQuantity();
      bases.push_back(json::Object{{"key", rkey(c, bd)}, {"off", off}, {"virtual", b.isVirtual()},
                                   {"size", c.ac->getTypeSizeInChars(b.getType()).getQuantity()}});
    }
  }
  for (const FieldDecl *fd : rd->fields()) {
    QualType t = fd->getType();
    if (t->isIncompleteType() || t->isDependentType()) continue;
    uint64_t bits = L.getFieldOffset(fd->getFieldIndex());
    QualType et = t;
    int64_t n = 0;
    if (const ConstantArrayType *at = c.ac->getAsConstantArrayType(t)) {
      n = 1;
      while (const ConstantArrayType *a2 = c.ac->getAsConstantArrayType(et)) {
        n *= a2->getSize().getZExtValue();
        et = a2->getElementType();
      }
      (void)at;
    }
    unsigned fl = 0, fc = 0;
    locf(c, fd->getLocation(), &fl, &fc);
    json::Object fo{{"name", fd->getName().str()}, {"off", (int64_t)(bits / 8)},
                    {"size", fd->isBitField() ? 0 : c.ac->getTypeSizeInChars(t).getQuantity()},
                    {"type", tname(c, t)}, {"cat", std::string(1, cat(et))}, {"line", fl}};
    if (n) {
      fo["n"] = n;
      fo["esz"] = c.ac->getTypeSizeInChars(et).getQuantity();
    }
    if (fd->isBitField()) fo["bit"] = (int64_t)(bits % 8);
    if (const RecordDecl *er = et->getAsRecordDecl()) {
      if (er->getDefinition()) {
        fo["rec"] = rkey(c, er->getDefinition());
        fo["hasptr"] = hasPtr(c, et);
      }
    }
    fields.push_back(std::move(fo));
  }
  o["bases"] = std::move(bases);
  o["fields"] = std::move(fields);
  // vtable slots: MSVC rule (each new virtual one slot in declaration order, an override reuses its base's slot, the
  // destructor one slot) vs the Itanium indices clang assigns (the destructor takes two)
  if (cx && cx->isPolymorphic() && !c.ac->getTargetInfo().getCXXABI().isMicrosoft()) {
    std::vector<const CXXRecordDecl *> chain;
    for (const CXXRecordDecl *r = cx; r;) {
      chain.push_back(r);
      const CXXRecordDecl *pb = nullptr;
      const ASTRecordLayout &RL = c.ac->getASTRecordLayout(r);
      pb = RL.getPrimaryBase();
      r = pb;
    }
    std::reverse(chain.begin(), chain.end());
    std::vector<const CXXMethodDecl *> ms;  // msvc slot -> method (most derived override)
    for (const CXXRecordDecl *r : chain) {
      for (const CXXMethodDecl *m : r->methods()) {
        if (!m->isVirtual()) continue;
        bool over = false;
        for (size_t i = 0; i < ms.size(); i++) {
          const CXXMethodDecl *b = ms[i];
          bool same = (isa<CXXDestructorDecl>(m) && isa<CXXDestructorDecl>(b));
          if (!same)
            for (const CXXMethodDecl *ov : m->overridden_methods())
              if (ov->getCanonicalDecl() == b->getCanonicalDecl()) same = true;
          if (!same && !isa<CXXDestructorDecl>(m) && !isa<CXXDestructorDecl>(b))
            for (const CXXMethodDecl *ov : b->overridden_methods())
              (void)ov;
          if (same) {
            ms[i] = m;
            over = true;
            break;
          }
        }
        if (!over) ms.push_back(m);
      }
    }
    ItaniumVTableContext *vc = dyn_cast<ItaniumVTableContext>(c.ac->getVTableContext());
    if (vc) {
      for (size_t i = 0; i < ms.size(); i++) {
        const CXXMethodDecl *m = ms[i];
        GlobalDecl gd = isa<CXXDestructorDecl>(m) ? GlobalDecl(cast<CXXDestructorDecl>(m), Dtor_Complete)
                                                  : GlobalDecl(m);
        // index in the most derived class's vtable: the method's own (or its first overridden) slot
        const CXXMethodDecl *base = m;
        while (base->size_overridden_methods()) base = *base->begin_overridden_methods();
        if (isa<CXXDestructorDecl>(base))
          gd = GlobalDecl(cast<CXXDestructorDecl>(base), Dtor_Complete);
        else
          gd = GlobalDecl(base);
        uint64_t ita = vc->getMethodVTableIndex(gd);
        slots.push_back(json::Array{(int64_t)i, (int64_t)ita, m->getNameAsString(),
                                    isa<CXXDestructorDecl>(m) ? 1 : 0});
      }
    }
    o["slots"] = std::move(slots);
  }
  g_records.push_back(std::move(o));
}

class V : public RecursiveASTVisitor<V> {
 public:
  Ctx &c;
  std::vector<std::string> fstack;
  std::map<const VarDecl *, const RecordDecl *> charAlias;  // char* e = (char*)rec;
  explicit V(Ctx &cc) : c(cc) {}
  bool shouldVisitTemplateInstantiations() const { return false; }
  bool shouldVisitImplicitCode() const { return false; }

  bool TraverseFunctionDecl(FunctionDecl *fd) { return trav(fd, [&] { return RecursiveASTVisitor::TraverseFunctionDecl(fd); }); }
  bool TraverseCXXMethodDecl(CXXMethodDecl *fd) { return trav(fd, [&] { return RecursiveASTVisitor::TraverseCXXMethodDecl(fd); }); }
  bool TraverseCXXConstructorDecl(CXXConstructorDecl *fd) { return trav(fd, [&] { return RecursiveASTVisitor::TraverseCXXConstructorDecl(fd); }); }
  bool TraverseCXXDestructorDecl(CXXDestructorDecl *fd) { return trav(fd, [&] { return RecursiveASTVisitor::TraverseCXXDestructorDecl(fd); }); }
  template <class F> bool trav(FunctionDecl *fd, F f) {
    fstack.push_back(fd->getQualifiedNameAsString());
    bool r = f();
    fstack.pop_back();
    return r;
  }

  bool VisitRecordDecl(RecordDecl *rd) {
    emitRecord(c, rd);
    return true;
  }
  bool VisitClassTemplateSpecializationDecl(ClassTemplateSpecializationDecl *rd) {
    return true;  // instantiations are emitted when a field / site refers to them (see need())
  }

  std::set<const RecordDecl *> needed;
  void need(const RecordDecl *rd) {
    if (rd && (rd = rd->getDefinition()) && isa<ClassTemplateSpecializationDecl>(rd)) needed.insert(rd);
  }

  bool site(SourceLocation l, json::Object o) {
    if (OptMode != 64) return false;
    unsigned line = 0, col = 0;
    std::string f = locf(c, l, &line, &col);
    if (f.empty() || f.rfind("src/", 0) != 0) return false;
    o["file"] = f;
    o["line"] = line;
    o["col"] = col;
    o["func"] = fstack.empty() ? "" : fstack.back();
    if (l.isMacroID()) o["macro"] = Lexer::getImmediateMacroNameForDiagnostics(l, *c.sm, c.ac->getLangOpts()).str();
    g_sites.push_back(std::move(o));
    return true;
  }

  const Expr *up(const Expr *e, const Stmt **parentOut = nullptr) {
    // the enclosing expression, skipping parens and implicit casts
    const Stmt *s = e;
    while (true) {
      auto ps = c.ac->getParents(*s);
      if (ps.empty()) return nullptr;
      const Stmt *p = ps[0].get<Stmt>();
      if (!p) {
        if (parentOut) *parentOut = nullptr;
        return nullptr;
      }
      if (isa<ParenExpr>(p) || isa<ImplicitCastExpr>(p)) {
        s = p;
        continue;
      }
      if (parentOut) *parentOut = p;
      return dyn_cast<Expr>(p);
    }
  }
  DynTypedNode upNode(const Stmt *s) {
    while (true) {
      auto ps = c.ac->getParents(*s);
      if (ps.empty()) return DynTypedNode();
      const Stmt *p = ps[0].get<Stmt>();
      if (p && (isa<ParenExpr>(p) || isa<ImplicitCastExpr>(p) || isa<ExprWithCleanups>(p))) {
        s = p;
        continue;
      }
      return ps[0];
    }
  }

  const RecordDecl *recOf(QualType pt) {
    if (!pt->isPointerType()) return nullptr;
    QualType q = pt->getPointeeType();
    const RecordDecl *r = q->getAsRecordDecl();
    return r ? r->getDefinition() : nullptr;
  }
  bool related(const RecordDecl *a, const RecordDecl *b) {
    if (a == b) return true;
    auto *ca = dyn_cast<CXXRecordDecl>(a), *cb = dyn_cast<CXXRecordDecl>(b);
    if (!ca || !cb || !ca->hasDefinition() || !cb->hasDefinition()) return false;
    return ca->isDerivedFrom(cb) || cb->isDerivedFrom(ca);
  }
  bool isCharLike(QualType t) {
    t = t.getCanonicalType().getUnqualifiedType();
    return t->isCharType() || t->isVoidType() || t->isSpecificBuiltinType(BuiltinType::UChar) ||
           t->isSpecificBuiltinType(BuiltinType::SChar);
  }
  std::optional<int64_t> cint(const Expr *e) {
    if (!e || e->isValueDependent()) return std::nullopt;
    Expr::EvalResult r;
    if (e->EvaluateAsInt(r, *c.ac)) return r.Val.getInt().getExtValue();
    return std::nullopt;
  }
  // the type the byte address is finally used as: (T*)((char*)p + N) -> T*
  std::string accessType(const Expr *e) {
    const Expr *p = up(e);
    while (p) {
      if (auto *ce = dyn_cast<ExplicitCastExpr>(p)) return tname(c, ce->getType());
      if (isa<BinaryOperator>(p) || isa<ArraySubscriptExpr>(p)) return "char";
      p = up(p);
      if (!p) break;
    }
    return "";
  }
  // where an integer value goes / comes from: a member, a parameter, a local, a return
  void attrib(json::Object &o, const Expr *e, bool dest) {
    if (dest) {
      // the cast's value is assigned / initialises / is passed / returned
      DynTypedNode n = upNode(e);
      const Stmt *ps = n.get<Stmt>();
      if (ps) {
        if (auto *bo = dyn_cast<BinaryOperator>(ps)) {
          if (bo->isAssignmentOp()) holder(o, bo->getLHS());
        } else if (auto *ce = dyn_cast<CallExpr>(ps)) {
          if (const FunctionDecl *fd = ce->getDirectCallee()) {
            for (unsigned i = 0; i < ce->getNumArgs(); i++)
              if (ce->getArg(i)->IgnoreParenImpCasts() == e->IgnoreParenImpCasts() && i < fd->getNumParams()) {
                o["holder"] = "param";
                o["hname"] = fd->getQualifiedNameAsString() + "#" + std::to_string(i) + " " +
                             fd->getParamDecl(i)->getNameAsString();
                o["htype"] = tname(c, fd->getParamDecl(i)->getType());
              }
          }
        } else if (isa<ReturnStmt>(ps)) {
          o["holder"] = "return";
          o["hname"] = fstack.empty() ? "" : fstack.back();
        } else if (auto *il = dyn_cast<InitListExpr>(ps)) {
          (void)il;
          o["holder"] = "init";
        }
      } else if (const VarDecl *vd = n.get<VarDecl>()) {
        o["holder"] = vd->isLocalVarDeclOrParm() ? "local" : "global";
        o["hname"] = vd->getQualifiedNameAsString();
        o["htype"] = tname(c, vd->getType());
      } else if (const FieldDecl *fd = n.get<FieldDecl>()) {
        holderField(o, fd);
      } else if (n.get<CXXCtorInitializer>()) {
        const CXXCtorInitializer *ci = n.get<CXXCtorInitializer>();
        if (ci->getMember()) holderField(o, ci->getMember());
      }
    } else {
      holder(o, e);
    }
  }
  void holder(json::Object &o, const Expr *e) {
    e = e->IgnoreParenImpCasts();
    if (auto *as = dyn_cast<ArraySubscriptExpr>(e)) e = as->getBase()->IgnoreParenImpCasts();
    if (auto *me = dyn_cast<MemberExpr>(e)) {
      if (auto *fd = dyn_cast<FieldDecl>(me->getMemberDecl())) holderField(o, fd);
    } else if (auto *dr = dyn_cast<DeclRefExpr>(e)) {
      if (auto *vd = dyn_cast<VarDecl>(dr->getDecl())) {
        o["holder"] = isa<ParmVarDecl>(vd) ? "param" : vd->isLocalVarDecl() ? "local" : "global";
        o["hname"] = vd->getQualifiedNameAsString();
        o["htype"] = tname(c, vd->getType());
      }
    } else if (auto *ce = dyn_cast<CallExpr>(e)) {
      if (const FunctionDecl *fd = ce->getDirectCallee()) {
        o["holder"] = "return";
        o["hname"] = fd->getQualifiedNameAsString();
        o["htype"] = tname(c, fd->getReturnType());
      }
    }
  }
  void holderField(json::Object &o, const FieldDecl *fd) {
    const RecordDecl *rd = fd->getParent();
    o["holder"] = "field";
    o["hrec"] = rkey(c, rd);
    o["hname"] = fd->getNameAsString();
    o["htype"] = tname(c, fd->getType());
    need(rd);
    if (rd->isCompleteDefinition() && !rd->isDependentType())
      o["hoff"] = (int64_t)(c.ac->getASTRecordLayout(rd).getFieldOffset(fd->getFieldIndex()) / 8);
  }

  bool VisitVarDecl(VarDecl *vd) {
    if (!vd->isLocalVarDecl() || !vd->getType()->isPointerType() || !isCharLike(vd->getType()->getPointeeType()))
      return true;
    if (const Expr *in = vd->getInit()) {
      in = in->IgnoreParenImpCasts();
      if (auto *ce = dyn_cast<ExplicitCastExpr>(in))
        if (const RecordDecl *r = recOf(ce->getSubExpr()->IgnoreParenImpCasts()->getType())) charAlias[vd] = r;
    }
    return true;
  }

  void rawOff(const Expr *at, const RecordDecl *r, const Expr *base, const Expr *offe, bool sub, const char *how) {
    json::Object o{{"kind", "rawoff"}, {"rec", rkey(c, r)}, {"how", how}};
    need(r);
    auto v = cint(offe);
    if (v) o["off"] = sub ? -*v : *v;
    else o["off"] = nullptr;
    o["access"] = accessType(at);
    o["text"] = text(at);
    (void)base;
    site(at->getBeginLoc(), std::move(o));
  }
  std::string text(const Expr *e) {
    CharSourceRange r = CharSourceRange::getTokenRange(c.sm->getFileLoc(e->getBeginLoc()), c.sm->getFileLoc(e->getEndLoc()));
    std::string s = Lexer::getSourceText(r, *c.sm, c.ac->getLangOpts()).str();
    if (s.size() > 160) s = s.substr(0, 160);
    return s;
  }

  // (char*)rec + N [+ i * K ...]  and  aliasing char* locals; reported once, at the outermost addition
  bool isCharAdd(const Expr *e) {
    auto *bo = dyn_cast<BinaryOperator>(e->IgnoreParens());
    return bo && (bo->getOpcode() == BO_Add || bo->getOpcode() == BO_Sub) && bo->getType()->isPointerType() &&
           isCharLike(bo->getType()->getPointeeType());
  }
  static bool usesSizeof(const Stmt *s) {
    if (!s) return false;
    if (isa<UnaryExprOrTypeTraitExpr>(s) || isa<OffsetOfExpr>(s)) return true;
    for (const Stmt *ch : s->children())
      if (usesSizeof(ch)) return true;
    return false;
  }
  bool VisitBinaryOperator(BinaryOperator *bo) {
    if (!isCharAdd(bo)) return true;
    const Expr *p = up(bo);
    if (p && isCharAdd(p)) {
      auto *pb = cast<BinaryOperator>(p->IgnoreParens());
      if (pb->getLHS()->IgnoreParenImpCasts() == bo) return true;
    }
    int64_t off = 0;
    bool known = true, usedSizeof = false;
    json::Array strides;
    const Expr *cur = bo;
    const RecordDecl *r = nullptr;
    const char *how = "cast";
    while (true) {
      const Expr *e = cur->IgnoreParenImpCasts();
      if (isCharAdd(e)) {
        auto *b = cast<BinaryOperator>(e);
        const Expr *rhs = b->getRHS()->IgnoreParenImpCasts();
        int sign = b->getOpcode() == BO_Sub ? -1 : 1;
        if (usesSizeof(rhs)) usedSizeof = true;
        if (auto v = cint(rhs)) off += sign * *v;
        else {
          known = false;
          if (auto *m = dyn_cast<BinaryOperator>(rhs)) {
            if (m->getOpcode() == BO_Mul) {
              if (auto k = cint(m->getRHS())) strides.push_back(*k);
              else if (auto k2 = cint(m->getLHS())) strides.push_back(*k2);
            } else if (m->getOpcode() == BO_Add) {
              // i * K + C
              const Expr *a = m->getLHS()->IgnoreParenImpCasts(), *cc = m->getRHS()->IgnoreParenImpCasts();
              if (auto cv = cint(cc)) {
                off += sign * *cv;
                if (auto *mm = dyn_cast<BinaryOperator>(a))
                  if (mm->getOpcode() == BO_Mul)
                    if (auto k = cint(mm->getRHS())) strides.push_back(*k);
              }
            }
          }
        }
        cur = b->getLHS();
        continue;
      }
      if (auto *ce = dyn_cast<ExplicitCastExpr>(e)) {
        r = recOf(ce->getSubExpr()->IgnoreParenImpCasts()->getType());
      } else if (auto *dr = dyn_cast<DeclRefExpr>(e)) {
        if (auto *vd = dyn_cast<VarDecl>(dr->getDecl())) {
          auto it = charAlias.find(vd);
          if (it != charAlias.end()) {
            r = it->second;
            how = "alias";
          }
        }
      }
      break;
    }
    if (!r) return true;
    json::Object o{{"kind", "rawoff"}, {"rec", rkey(c, r)}, {"how", how}};
    need(r);
    o["off"] = off;
    o["const"] = known;
    if (usedSizeof) o["sizeof"] = true;
    if (!strides.empty()) o["strides"] = std::move(strides);
    o["access"] = accessType(bo);
    o["text"] = text(bo);
    site(bo->getBeginLoc(), std::move(o));
    return true;
  }
  bool VisitArraySubscriptExpr(ArraySubscriptExpr *as) {
    const Expr *b = as->getBase()->IgnoreParenImpCasts();
    auto *ce = dyn_cast<ExplicitCastExpr>(b);
    if (!ce || !b->getType()->isPointerType()) return true;
    const RecordDecl *r = recOf(ce->getSubExpr()->IgnoreParenImpCasts()->getType());
    if (!r) return true;
    QualType et = b->getType()->getPointeeType();
    if (isCharLike(et)) {
      rawOff(as, r, ce->getSubExpr(), as->getIdx(), false, "index");
      return true;
    }
    if (et->isRecordType()) return true;  // a record array view: the recast site covers it
    json::Object o{{"kind", "scalar_index"}, {"rec", rkey(c, r)}, {"elem", tname(c, et)},
                   {"esz", c.ac->getTypeSizeInChars(et).getQuantity()}, {"ecat", std::string(1, cat(et))}};
    need(r);
    auto v = cint(as->getIdx());
    if (v) o["idx"] = *v;
    else o["idx"] = nullptr;
    o["text"] = text(as);
    site(as->getBeginLoc(), std::move(o));
    return true;
  }
  bool VisitUnaryOperator(UnaryOperator *uo) {
    // *(T*)rec : the first word of a record read as a scalar (the vptr when the record is polymorphic)
    if (uo->getOpcode() != UO_Deref) return true;
    auto *ce = dyn_cast<ExplicitCastExpr>(uo->getSubExpr()->IgnoreParenImpCasts());
    if (!ce || !ce->getType()->isPointerType()) return true;
    const RecordDecl *r = recOf(ce->getSubExpr()->IgnoreParenImpCasts()->getType());
    QualType et = ce->getType()->getPointeeType();
    if (!r || et->isRecordType() || isCharLike(et)) return true;
    json::Object o{{"kind", "scalar_index"}, {"rec", rkey(c, r)}, {"elem", tname(c, et)}, {"idx", 0},
                   {"esz", c.ac->getTypeSizeInChars(et).getQuantity()}, {"ecat", std::string(1, cat(et))},
                   {"text", text(uo)}};
    need(r);
    site(uo->getBeginLoc(), std::move(o));
    return true;
  }

  // the member a pointer expression designates: &X->f, &X->f[i], X->f (an array member decaying)
  const FieldDecl *memberOf(const Expr *sub, const Expr **idx) {
    *idx = nullptr;
    const Expr *m = nullptr;
    if (auto *uo = dyn_cast<UnaryOperator>(sub)) {
      if (uo->getOpcode() != UO_AddrOf) return nullptr;
      m = uo->getSubExpr()->IgnoreParenImpCasts();
      if (auto *as = dyn_cast<ArraySubscriptExpr>(m)) {
        *idx = as->getIdx();
        m = as->getBase()->IgnoreParenImpCasts();
      }
    } else if (sub->getType()->isArrayType()) {
      m = sub;
    } else {
      return nullptr;
    }
    auto *me = dyn_cast<MemberExpr>(m);
    if (!me) return nullptr;
    return dyn_cast<FieldDecl>(me->getMemberDecl());
  }

  bool VisitExplicitCastExpr(ExplicitCastExpr *ce) {
    QualType to = ce->getType();
    const Expr *sub = ce->getSubExpr()->IgnoreParenImpCasts();
    QualType from = sub->getType();
    CastKind k = ce->getCastKind();
    if (k == CK_NoOp || k == CK_LValueToRValue) {
      // a C-style cast's kind is the last step; look through its implicit chain
      if (auto *ic = dyn_cast<ImplicitCastExpr>(ce->getSubExpr())) k = ic->getCastKind();
    }
    if (from->isPointerType() || from->isArrayType() || from->isFunctionType() || from->isNullPtrType()) {
      if (to->isIntegerType() && !to->isBooleanType()) {
        uint64_t w = c.ac->getTypeSize(to);
        json::Object o{{"kind", "p2i"}, {"to", tname(c, to)}, {"from", tname(c, from)}, {"width", (int64_t)w},
                       {"text", text(ce)}};
        if (const RecordDecl *r = recOf(from)) {
          o["fromrec"] = rkey(c, r);
          need(r);
        }
        if (sub->isNullPointerConstant(*c.ac, Expr::NPC_ValueDependentIsNull)) return true;
        attrib(o, ce, true);
        site(ce->getBeginLoc(), std::move(o));
        return true;
      }
    }
    if (from->isIntegerType() && !from->isBooleanType() && (to->isPointerType() || to->isFunctionPointerType())) {
      if (sub->isNullPointerConstant(*c.ac, Expr::NPC_ValueDependentIsNull)) return true;
      if (cint(sub)) return true;  // (T*)0x5e2930 style image addresses: listed by the standalone gates
      uint64_t w = c.ac->getTypeSize(from);
      json::Object o{{"kind", "i2p"}, {"to", tname(c, to)}, {"from", tname(c, from)}, {"width", (int64_t)w},
                     {"text", text(ce)}};
      if (from->isEnumeralType()) o["enum"] = true;
      attrib(o, sub, false);
      site(ce->getBeginLoc(), std::move(o));
      return true;
    }
    if (from->isArrayType() && to->isPointerType()) from = c.ac->getArrayDecayedType(from);
    if (from->isPointerType() && to->isPointerType()) {
      QualType fp = from->getPointeeType(), tp = to->getPointeeType();
      const RecordDecl *tr = tp->getAsRecordDecl();
      if (tr) tr = tr->getDefinition();
      if (!tr) {
        // *(T**)&X->field / (int*)&X->pad[4]: a member's bytes read as another scalar type
        if (tp->isVoidType() || isCharLike(tp)) return true;
        const Expr *idx = nullptr;
        const FieldDecl *fd = memberOf(sub, &idx);
        if (!fd) return true;
        QualType ft = fd->getType();
        while (const ArrayType *at = c.ac->getAsArrayType(ft)) ft = at->getElementType();
        if (ft.getCanonicalType().getUnqualifiedType() == tp.getCanonicalType().getUnqualifiedType()) return true;
        json::Object o{{"kind", "fieldpun"}, {"to", tname(c, tp)}, {"tcat", std::string(1, cat(tp))},
                       {"tsize", tp->isIncompleteType() ? 0 : c.ac->getTypeSizeInChars(tp).getQuantity()},
                       {"text", text(ce)}};
        holderField(o, fd);
        if (idx) {
          auto v = cint(idx);
          o["idx"] = v ? json::Value(*v) : json::Value(nullptr);
          o["esz"] = c.ac->getTypeSizeInChars(ft).getQuantity();
        }
        site(ce->getBeginLoc(), std::move(o));
        return true;
      }
      // &X->field reinterpreted as a record
      {
        const Expr *idx = nullptr;
        if (const FieldDecl *fd = memberOf(sub, &idx)) {
          QualType ft = fd->getType();
          while (const ArrayType *at = c.ac->getAsArrayType(ft)) ft = at->getElementType();
          const RecordDecl *frd = ft->getAsRecordDecl();
          if (frd && frd->getDefinition() && related(frd->getDefinition(), tr)) return true;
          json::Object o{{"kind", "fieldcast"}, {"to", rkey(c, tr)}, {"text", text(ce)}};
          need(tr);
          holderField(o, fd);
          if (idx) {
            auto v = cint(idx);
            o["idx"] = v ? json::Value(*v) : json::Value(nullptr);
          }
          site(ce->getBeginLoc(), std::move(o));
          return true;
        }
      }
      const RecordDecl *fr = fp->getAsRecordDecl();
      if (fr) fr = fr->getDefinition();
      json::Object o{{"kind", "recast"}, {"to", rkey(c, tr)}, {"text", text(ce)}};
      need(tr);
      if (fr) {
        if (related(fr, tr)) return true;
        o["from"] = rkey(c, fr);
        need(fr);
      } else {
        if (tp.getCanonicalType() == fp.getCanonicalType()) return true;
        o["from"] = "";
        o["fromtype"] = tname(c, fp);
        // the expression the raw pointer came from: (T*)((char*)p + N) is a raw offset (listed there)
        const Expr *s2 = sub;
        if (auto *bo = dyn_cast<BinaryOperator>(s2)) {
          (void)bo;
          return true;
        }
        holder(o, sub);
      }
      // the call made through the reinterpreted pointer (vtable views)
      const Expr *p = up(ce);
      if (p)
        if (auto *me = dyn_cast<MemberExpr>(p))
          if (auto *md = dyn_cast<CXXMethodDecl>(me->getMemberDecl()))
            if (md->isVirtual()) o["vcall"] = md->getNameAsString();
      site(ce->getBeginLoc(), std::move(o));
    }
    return true;
  }

  std::map<const VarDecl *, std::pair<int64_t, std::string>> rawMem;
  // new (mem) T(...) over a block from operator new(0x7c): the block's size against sizeof(T)
  bool VisitCXXNewExpr(CXXNewExpr *ne) {
    if (ne->getNumPlacementArgs() != 1) return true;
    const Expr *a = ne->getPlacementArg(0)->IgnoreParenImpCasts();
    auto *dr = dyn_cast<DeclRefExpr>(a);
    if (!dr) return true;
    auto *vd = dyn_cast<VarDecl>(dr->getDecl());
    auto it = vd ? rawMem.find(vd) : rawMem.end();
    if (it == rawMem.end()) return true;
    const RecordDecl *r = ne->getAllocatedType()->getAsRecordDecl();
    if (!r || !(r = r->getDefinition())) return true;
    json::Object o{{"kind", "alloc"}, {"fn", it->second.second + " + placement new"}, {"size", it->second.first},
                   {"rec", rkey(c, r)}, {"text", text(ne)}};
    need(r);
    site(ne->getBeginLoc(), std::move(o));
    return true;
  }

  bool VisitCallExpr(CallExpr *ce) {
    const FunctionDecl *fd = ce->getDirectCallee();
    if (fd) {
      std::string n = fd->getNameAsString();
      bool alloc = n == "malloc" || n == "calloc" || n == "realloc" || n == "_aligned_malloc" || n == "HeapAlloc" ||
                   n == "GlobalAlloc" || n == "LocalAlloc" || n == "VirtualAlloc";
      if (fd->getDeclName().getNameKind() == DeclarationName::CXXOperatorName &&
          (fd->getOverloadedOperator() == OO_New || fd->getOverloadedOperator() == OO_Array_New))
        alloc = !isa<CXXNewExpr>(ce);
      bool mem = n == "memset" || n == "memcpy" || n == "memmove" || n == "ZeroMemory" || n == "CopyMemory" ||
                 n == "RtlZeroMemory" || n == "memcmp";
      if (alloc && ce->getNumArgs()) {
        unsigned ai = (n == "HeapAlloc") ? 2 : (n == "realloc") ? 1 : (n == "GlobalAlloc" || n == "LocalAlloc") ? 1 : 0;
        if (ai >= ce->getNumArgs()) return true;
        auto v = cint(ce->getArg(ai));
        if (!v) return true;
        json::Object o{{"kind", "alloc"}, {"fn", n}, {"size", *v}, {"text", text(ce)}};
        if (n == "calloc" && ce->getNumArgs() == 2)
          if (auto v2 = cint(ce->getArg(1))) o["size"] = *v * *v2;
        const Expr *p = up(ce);
        if (p)
          if (auto *xc = dyn_cast<ExplicitCastExpr>(p))
            if (const RecordDecl *r = recOf(xc->getType())) {
              o["rec"] = rkey(c, r);
              need(r);
            }
        if (!o.get("rec")) {
          DynTypedNode nn = upNode(ce);
          const VarDecl *vd = nn.get<VarDecl>();
          if (!vd)
            if (const Stmt *ps = nn.get<Stmt>())
              if (auto *bo = dyn_cast<BinaryOperator>(ps))
                if (bo->isAssignmentOp())
                  if (auto *dr = dyn_cast<DeclRefExpr>(bo->getLHS()->IgnoreParenImpCasts()))
                    vd = dyn_cast<VarDecl>(dr->getDecl());
          if (vd) {
            if (const RecordDecl *r = recOf(vd->getType())) {
              o["rec"] = rkey(c, r);
              need(r);
            } else {
              rawMem[vd] = std::make_pair(*v, n);   // a raw block: typed by a later placement new
            }
          }
        }
        site(ce->getBeginLoc(), std::move(o));
      } else if (mem && ce->getNumArgs() >= 2) {
        unsigned si = (n == "ZeroMemory" || n == "RtlZeroMemory") ? 1 : 2;
        if (si >= ce->getNumArgs()) return true;
        auto v = cint(ce->getArg(si));
        if (!v || *v <= 8) return true;
        const Expr *dst = ce->getArg(0)->IgnoreParenImpCasts();
        const RecordDecl *r = recOf(dst->getType());
        if (!r) {
          if (auto *uo = dyn_cast<UnaryOperator>(dst))
            if (uo->getOpcode() == UO_AddrOf) r = uo->getSubExpr()->getType()->getAsRecordDecl();
        }
        if (!r) return true;
        r = r->getDefinition();
        if (!r) return true;
        json::Object o{{"kind", "memsize"}, {"fn", n}, {"size", *v}, {"rec", rkey(c, r)}, {"text", text(ce)}};
        need(r);
        site(ce->getBeginLoc(), std::move(o));
      }
      return true;
    }
    if (auto *mc = dyn_cast<CXXMemberCallExpr>(ce)) {
      const CXXMethodDecl *md = mc->getMethodDecl();
      if (md && md->getNumParams() >= 1 && mc->getNumArgs() >= 1 && md->getParamDecl(0)->getType()->isIntegerType()) {
        const FunctionDecl *pat = md->getTemplateInstantiationPattern();
        if (!pat) pat = md;
        if (pat->hasBody()) {
          std::string body = Lexer::getSourceText(CharSourceRange::getTokenRange(pat->getBody()->getSourceRange()),
                                                  *c.sm, c.ac->getLangOpts()).str();
          if (body.find("(char*)this +") != std::string::npos || body.find("(char *)this +") != std::string::npos) {
            if (auto v = cint(mc->getArg(0))) {
              const RecordDecl *r = md->getParent();
              json::Object o{{"kind", "rawoff"}, {"rec", rkey(c, r)}, {"how", "helper"}, {"off", *v},
                             {"const", true}, {"access", tname(c, md->getReturnType())}, {"text", text(ce)}};
              need(r);
              site(ce->getBeginLoc(), std::move(o));
            }
          }
        }
      }
      return true;
    }
    if (isa<CXXOperatorCallExpr>(ce)) return true;
    // a call through a computed function pointer: vtable slot numbers, function-pointer tables read raw
    const Expr *cal = ce->getCallee()->IgnoreParenImpCasts();
    if (auto *uo = dyn_cast<UnaryOperator>(cal))
      if (uo->getOpcode() == UO_Deref) cal = uo->getSubExpr()->IgnoreParenImpCasts();
    bool computed = false;
    std::optional<int64_t> slot;
    std::function<void(const Stmt *)> scan = [&](const Stmt *s) {
      if (!s) return;
      if (auto *as = dyn_cast<ArraySubscriptExpr>(s)) {
        computed = true;
        if (!slot) slot = cint(as->getIdx());
      } else if (auto *bo = dyn_cast<BinaryOperator>(s)) {
        if (bo->isAdditiveOp() && bo->getType()->isPointerType()) {
          computed = true;
          if (!slot) slot = cint(bo->getRHS());
        }
      } else if (auto *xc = dyn_cast<ExplicitCastExpr>(s)) {
        QualType t = xc->getType();
        if (t->isPointerType() && t->getPointeeType()->isPointerType() &&
            t->getPointeeType()->getPointeeType()->isFunctionType())
          computed = true;
      }
      for (const Stmt *ch : s->children()) scan(ch);
    };
    scan(cal);
    if (!computed) return true;
    json::Object o{{"kind", "vslot"}, {"text", text(ce->getCallee())}};
    o["slot"] = slot ? json::Value(*slot) : json::Value(nullptr);
    site(ce->getBeginLoc(), std::move(o));
    return true;
  }
};

class Consumer : public ASTConsumer {
 public:
  void HandleTranslationUnit(ASTContext &ac) override {
    Ctx c{&ac, &ac.getSourceManager(), PrintingPolicy(ac.getLangOpts())};
    c.pp.SuppressTagKeyword = true;
    c.pp.FullyQualifiedName = true;
    c.pp.PrintAsCanonical = true;
    V v(c);
    v.TraverseDecl(ac.getTranslationUnitDecl());
    // template instantiations a site or field used (Pool<Agent>, std::vector<T>) and the records fields name
    std::set<const RecordDecl *> done;
    std::vector<const RecordDecl *> work(v.needed.begin(), v.needed.end());
    while (!work.empty()) {
      const RecordDecl *rd = work.back();
      work.pop_back();
      if (!done.insert(rd).second) continue;
      if (rd->isDependentType() || !rd->isCompleteDefinition()) continue;
      emitRecord(c, rd);
      for (const FieldDecl *f : rd->fields()) {
        QualType t = f->getType();
        while (const ArrayType *at = ac.getAsArrayType(t)) t = at->getElementType();
        if (const RecordDecl *r = t->getAsRecordDecl())
          if ((r = r->getDefinition()) && isa<ClassTemplateSpecializationDecl>(r)) work.push_back(r);
      }
      if (auto *cx = dyn_cast<CXXRecordDecl>(rd))
        for (auto &b : cx->bases())
          if (const CXXRecordDecl *bd = b.getType()->getAsCXXRecordDecl())
            if ((bd = bd->getDefinition()) && isa<ClassTemplateSpecializationDecl>(bd)) work.push_back(bd);
    }
    SourceManager &sm = ac.getSourceManager();
    for (auto it = sm.fileinfo_begin(); it != sm.fileinfo_end(); ++it) g_files.insert(it->first.getName().str());
  }
};

class Action : public ASTFrontendAction {
 public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &ci, StringRef) override {
    ci.getDiagnostics().setClient(new Diags, true);
    ci.getDiagnostics().setErrorLimit(0);
    return std::make_unique<Consumer>();
  }
};

int main(int argc, const char **argv) {
  // split at "--": our options before, the clang driver command after
  int dd = argc;
  for (int i = 1; i < argc; i++)
    if (std::string(argv[i]) == "--") {
      dd = i;
      break;
    }
  llvm::cl::ParseCommandLineOptions(dd, argv, "port64_ast\n");
  std::vector<std::string> args;
  for (int i = dd + 1; i < argc; i++) args.push_back(argv[i]);
  if (!OptRoot.empty() && OptRoot.back() != '/') OptRoot += "/";
  tooling::FixedCompilationDatabase db(".", args);
  tooling::ClangTool tool(db, std::vector<std::string>(OptSrc.begin(), OptSrc.end()));
  tool.appendArgumentsAdjuster(tooling::getClangSyntaxOnlyAdjuster());
  class F : public tooling::FrontendActionFactory {
   public:
    std::unique_ptr<FrontendAction> create() override { return std::make_unique<Action>(); }
  } f;
  int rc = tool.run(&f);
  json::Array files;
  for (auto &s : g_files) files.push_back(s);
  json::Object out{{"mode", (int)OptMode}, {"rc", rc}, {"files", std::move(files)}, {"records", std::move(g_records)},
                   {"sites", std::move(g_sites)}, {"diags", std::move(g_diags)}};
  std::error_code ec;
  llvm::raw_fd_ostream os(OptOut, ec);
  if (ec) {
    llvm::errs() << "cannot write " << OptOut << "\n";
    return 2;
  }
  os << json::Value(std::move(out));
  return 0;
}
