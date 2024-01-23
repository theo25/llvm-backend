#include <kllvm/ast/AST.h>

#include <kllvm/parser/KOREParser.h>
#include <kllvm/util/transitive_closure.h>

#include <string>
#include <unordered_set>

namespace kllvm {

ptr<KOREDefinition> KOREDefinition::load(std::string const &filename) {
  auto definition = parser::KOREParser(filename).definition();
  definition->precompute();
  return definition;
}

void KOREDefinition::precompute() {
  buildSubsortRelation();
  buildOverloadRelation();
  buildSortContainsRelation();
}

std::unordered_set<std::string>
KOREDefinition::getSortsHookedTo(std::string const &hookName) const {
  auto ret = std::unordered_set<std::string>{};

  for (auto const &[name, decl] : getSortDeclarations()) {
    if (decl->isHooked()) {
      if (auto hook = decl->getStringAttribute("hook"); hook == hookName) {
        ret.insert(name);
      }
    }
  }

  return ret;
}

void KOREDefinition::addModule(sptr<KOREModule> Module) {
  for (auto const &decl : Module->getDeclarations()) {
    if (auto *sortDecl
        = dynamic_cast<KORECompositeSortDeclaration *>(decl.get())) {
      sortDeclarations.insert({sortDecl->getName(), sortDecl});
      auto sort = KORECompositeSort::Create(sortDecl->getName());
    } else if (
        auto *symbolDecl = dynamic_cast<KORESymbolDeclaration *>(decl.get())) {
      symbolDeclarations.insert(
          {symbolDecl->getSymbol()->getName(), symbolDecl});
    } else if (
        auto *aliasDecl = dynamic_cast<KOREAliasDeclaration *>(decl.get())) {
      aliasDeclarations.insert({aliasDecl->getSymbol()->getName(), aliasDecl});
    } else if (auto *axiom = dynamic_cast<KOREAxiomDeclaration *>(decl.get())) {
      axioms.push_back(axiom);
    }
  }
  modules.push_back(std::move(Module));
}

void KOREDefinition::addAttribute(sptr<KORECompositePattern> Attribute) {
  std::string name = Attribute->getConstructor()->getName();
  attributes.insert({name, std::move(Attribute)});
}

void KOREDefinition::insertReservedSymbols() {
  auto mod = KOREModule::Create("K-RAW-TERM");
  auto decl = KORESymbolDeclaration::Create("rawTerm", true);
  auto sort = KORECompositeSort::Create("SortKItem");

  decl->getSymbol()->addSort(sort);
  decl->getSymbol()->addArgument(sort);
  mod->addDeclaration(std::move(decl));

  addModule(std::move(mod));
}

void KOREDefinition::buildSubsortRelation() {
  for (auto *axiom : axioms) {
    if (axiom->getAttributes().count("subsort")) {
      auto const &att = axiom->getAttributes().at("subsort");
      auto const &innerSort = att->getConstructor()->getFormalArguments()[0];
      auto const &outerSort = att->getConstructor()->getFormalArguments()[1];
      subsorts[innerSort.get()].insert(outerSort.get());
      supersorts[outerSort.get()].insert(innerSort.get());
    }
  }

  subsorts = transitive_closure(subsorts);
  supersorts = transitive_closure(supersorts);
}

void KOREDefinition::buildOverloadRelation() {
  for (auto *axiom : axioms) {
    if (axiom->getAttributes().count("overload")) {
      auto const &att = axiom->getAttributes().at("overload");
      auto *innerSymbol = std::dynamic_pointer_cast<KORECompositePattern>(
                              att->getArguments()[1])
                              ->getConstructor();
      auto *outerSymbol = std::dynamic_pointer_cast<KORECompositePattern>(
                              att->getArguments()[0])
                              ->getConstructor();
      overloads[innerSymbol].insert(outerSymbol);
    }
  }

  overloads = transitive_closure(overloads);
}

void KOREDefinition::buildSortContainsRelation() {
  auto const &supersorts = getSupersorts();

  for (auto const &[name, decl] : getSymbolDeclarations()) {
    if (decl->isFunction() && !decl->isCollectionElement()) {
      continue;
    }

    auto *symbol = decl->getSymbol();
    auto const &return_sort
        = std::dynamic_pointer_cast<KORECompositeSort>(symbol->getSort());
    if (!return_sort) {
      continue;
    }

    auto &children = sortContains[return_sort.get()];
    children.insert(return_sort.get());

    for (auto const &arg : symbol->getArguments()) {
      children.insert(arg.get());
    }

    auto subsorts = std::vector<KORESort *>{};
    for (auto const &child_sort : children) {
      if (supersorts.find(child_sort) != supersorts.end()) {
        for (auto const &child_subsort : supersorts.at(child_sort)) {
          subsorts.push_back(child_subsort);
        }
      }
    }

    std::copy(
        subsorts.begin(), subsorts.end(),
        std::inserter(children, children.begin()));
  }

  sortContains = transitive_closure(sortContains);

  // Hooked sorts produce empty relations in the transitive closure because they
  // cannot occur as the return sort of a constructor symbol, and cannot be
  // subsorted. However, we want the relation to encode that a term of hooked
  // sort H _can_ contain a term of hooked sort H, and so we take the reflexive
  // closure of the transitive closure to ensure this.
  sortContains = reflexive_closure(sortContains);
}

// NOLINTNEXTLINE(*-function-cognitive-complexity)
void KOREDefinition::preprocess() {
  insertReservedSymbols();

  for (auto *axiom : axioms) {
    axiom->pattern = axiom->pattern->expandAliases(this);
  }
  auto symbols = std::map<std::string, std::vector<KORESymbol *>>{};
  unsigned nextOrdinal = 0;
  for (auto const &decl : symbolDeclarations) {
    if (decl.second->getAttributes().count("freshGenerator")) {
      auto sort = decl.second->getSymbol()->getSort();
      if (sort->isConcrete()) {
        freshFunctions[dynamic_cast<KORECompositeSort *>(sort.get())->getName()]
            = decl.second->getSymbol();
      }
    }
  }
  for (auto iter = axioms.begin(); iter != axioms.end();) {
    auto *axiom = *iter;
    axiom->ordinal = nextOrdinal;
    ordinals[nextOrdinal++] = axiom;
    axiom->pattern->markSymbols(symbols);
    if (!axiom->isRequired()) {
      iter = axioms.erase(iter);
    } else {
      ++iter;
    }
  }
  for (auto &module : modules) {
    auto const &declarations = module->getDeclarations();
    for (auto const &declaration : declarations) {
      auto *decl = dynamic_cast<KORESymbolDeclaration *>(declaration.get());
      if (decl == nullptr) {
        continue;
      }
      if (decl->isHooked() && decl->getObjectSortVariables().empty()) {
        KORESymbol *symbol = decl->getSymbol();
        symbols.emplace(symbol->getName(), std::vector<KORESymbol *>{symbol});
      }
    }
  }
  for (auto const &entry : symbols) {
    for (auto *symbol : entry.second) {
      auto *decl = symbolDeclarations.at(symbol->getName());
      symbol->instantiateSymbol(decl);
    }
  }
  uint32_t nextSymbol = 0;
  uint16_t nextLayout = 1;
  auto instantiations = std::unordered_map<KORESymbol, uint32_t, HashSymbol>{};
  auto layouts = std::unordered_map<std::string, uint16_t>{};
  auto variables
      = std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>{};
  for (auto const &entry : symbols) {
    uint32_t firstTag = nextSymbol;
    for (auto *symbol : entry.second) {
      if (symbol->isConcrete()) {
        if (!instantiations.count(*symbol)) {
          instantiations.emplace(*symbol, nextSymbol++);
        }
        std::string layoutStr = symbol->layoutString(this);
        if (!layouts.count(layoutStr)) {
          layouts.emplace(layoutStr, nextLayout++);
        }
        symbol->firstTag = symbol->lastTag = instantiations.at(*symbol);
        symbol->layout = layouts.at(layoutStr);
        objectSymbols[symbol->firstTag] = symbol;
        allObjectSymbols[ast_to_string(*symbol)] = symbol;
      }
    }
    uint32_t lastTag = nextSymbol - 1;
    if (!entry.second.empty()) {
      variables.emplace(
          entry.first, std::pair<uint32_t, uint32_t>{firstTag, lastTag});
    }
  }
  for (auto const &entry : symbols) {
    auto range = variables.at(entry.first);
    for (auto *symbol : entry.second) {
      for (auto const &sort : symbol->getArguments()) {
        if (sort->isConcrete()) {
          hookedSorts[dynamic_cast<KORECompositeSort *>(sort.get())
                          ->getCategory(this)]
              = std::dynamic_pointer_cast<KORECompositeSort>(sort);
        }
      }
      if (symbol->getSort()->isConcrete()) {
        hookedSorts[dynamic_cast<KORECompositeSort *>(symbol->getSort().get())
                        ->getCategory(this)]
            = std::dynamic_pointer_cast<KORECompositeSort>(symbol->getSort());
      }
      if (!symbol->isConcrete()) {
        if (symbol->isPolymorphic()) {
          symbol->firstTag = range.first;
          symbol->lastTag = range.second;
          auto *decl = symbolDeclarations.at(symbol->getName());
          if (decl->getAttributes().count("sortInjection")) {
            injSymbol = symbol;
          }
        }
      }
    }
  }
}

} // namespace kllvm
