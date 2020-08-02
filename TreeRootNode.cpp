#include "TreeNode.cpp"

class TreeRootNode : public TreeNode // Sets up the compilation context, rather
                                     // than compiling itself.
{
public:
  TreeRootNode() {
    text = "module"; // That's how the root node of the AST is called in
                     // WebAssembly and JavaScript.
  }
  AssemblyCode compile() {
    CompilationContext context;
    std::string globalDeclarations =
        R"(;;Generated by AECforWebAssembly ( https://github.com/FlatAssembler/AECforWebAssembly ).
(module
  (import "JavaScript" "memory" (memory 1)) ;;Assume that, in the JavaScript program linked to this
  ;;AEC program, there is an "extern"-ed bytearray of size 1 page (64KB) called "memory".
  ;;Have a better idea?
  (global $stack_pointer (import "JavaScript" "stack_pointer") (mut i32)) ;;The stack pointer being
  ;;visible from JavaScript will be useful when debugging the compiler.
)";
    auto allTheStrings = getStringsInSubnodes();
    for (auto string : allTheStrings) {
      context.globalVariables[string] = context.globalVariablePointer;
      if (string.back() != '"')
        string += '"';
      globalDeclarations += "\t(data 0 (i32.const " +
                            std::to_string(context.globalVariablePointer) +
                            ") " + string + ")\n";
      context.globalVariablePointer += string.size() - 1;
    }
    for (auto childNode : children) {
      if (basicDataTypeSizes.count(
              childNode.text)) { // Global variable declaration
        for (auto variableName : childNode.children) {
          if (!std::regex_match(variableName.text,
                                std::regex("^(_|[A-Z]|[a-z])\\w*\\[?$")) or
              AECkeywords.count(variableName.text)) {
            std::cerr
                << "Line " << variableName.lineNumber << ", Column "
                << variableName.columnNumber << ", Compiler error: \""
                << variableName.text
                << "\" is not a valid variable name! Aborting the compilation!"
                << std::endl;
            exit(1);
          }
          context.globalVariables[variableName.text] =
              context.globalVariablePointer;
          context.variableTypes[variableName.text] = childNode.text;
          if (variableName.text.back() == '[')
            context.globalVariablePointer +=
                basicDataTypeSizes[childNode.text] *
                variableName.children[0]
                    .interpretAsACompileTimeIntegerConstant();
          else
            context.globalVariablePointer += basicDataTypeSizes[childNode.text];
          globalDeclarations +=
              ";;\"" + variableName.text +
              "\" is declared to be at the memory address " +
              std::to_string(context.globalVariables[variableName.text]) + "\n";
          auto iteratorOfTheAssignment = std::find_if(
              variableName.children.begin(), variableName.children.end(),
              [](TreeNode node) { return node.text == ":="; });
          if (iteratorOfTheAssignment !=
              variableName.children.end()) { // If there is an initial value
                                             // assigned to the global variable.
            auto assignment = iteratorOfTheAssignment->children[0];
            if (assignment.text == "{}") { // Array initializers.
              if (variableName.text.back() != '[') {
                std::cerr
                    << "Line " << assignment.lineNumber << ", Column "
                    << assignment.columnNumber
                    << ", Compiler error: Can't assign an array to a variable "
                       "that's not an array. Aborting the compilation!"
                    << std::endl;
                exit(1);
              }
              int address = context.globalVariables[variableName.text];
              for (auto field : assignment.children) {
                if (childNode.text == "Character")
                  globalDeclarations +=
                      "\t(data 0 (i32.const " + std::to_string(address) + ") " +
                      getCharVectorRepresentationOfCharacter(
                          field.interpretAsACompileTimeIntegerConstant()) +
                      ") ;;Hex of " +
                      std::to_string(
                          field.interpretAsACompileTimeIntegerConstant()) +
                      "\n";
                else if (childNode.text == "Integer16")
                  globalDeclarations +=
                      "\t(data 0 (i32.const " + std::to_string(address) + ") " +
                      getCharVectorRepresentationOfInteger16(
                          field.interpretAsACompileTimeIntegerConstant()) +
                      ") ;;Hex of " +
                      std::to_string(
                          field.interpretAsACompileTimeIntegerConstant()) +
                      "\n";
                else if (childNode.text == "Integer32")
                  globalDeclarations +=
                      "\t(data 0 (i32.const " + std::to_string(address) + ") " +
                      getCharVectorRepresentationOfInteger32(
                          field.interpretAsACompileTimeIntegerConstant()) +
                      ") ;;Hex of " +
                      std::to_string(
                          field.interpretAsACompileTimeIntegerConstant()) +
                      "\n";
                else if (childNode.text == "Integer64")
                  globalDeclarations +=
                      "\t(data 0 (i32.const " + std::to_string(address) + ") " +
                      getCharVectorRepresentationOfInteger64(
                          field.interpretAsACompileTimeDecimalConstant()) +
                      ") ;;Hex of " +
                      std::to_string(
                          field.interpretAsACompileTimeIntegerConstant()) +
                      "\n";
                else if (childNode.text == "Decimal32")
                  globalDeclarations +=
                      "\t(data 0 (i32.const " + std::to_string(address) + ") " +
                      getCharVectorRepresentationOfDecimal32(
                          field.interpretAsACompileTimeDecimalConstant()) +
                      ") ;;IEEE 754 hex of " +
                      std::to_string(
                          field.interpretAsACompileTimeDecimalConstant()) +
                      "\n";
                else if (childNode.text == "Decimal64")
                  globalDeclarations +=
                      "\t(data 0 (i32.const " + std::to_string(address) + ") " +
                      getCharVectorRepresentationOfDecimal64(
                          field.interpretAsACompileTimeDecimalConstant()) +
                      ") ;;IEEE 754 hex of " +
                      std::to_string(
                          field.interpretAsACompileTimeDecimalConstant()) +
                      "\n";
                else {
                  std::cerr << "Line " << field.lineNumber << ", Column "
                            << field.columnNumber
                            << ", Internal compiler error: Compiler got into a "
                               "forbidden state!"
                            << std::endl;
                  exit(1);
                }
                address += basicDataTypeSizes[childNode.text];
              }
            } else if (assignment.text.front() == '"') // String.
            {
              if (childNode.text != "CharacterPointer") {
                std::cerr << "Line " << iteratorOfTheAssignment->lineNumber
                          << ", Column "
                          << iteratorOfTheAssignment->columnNumber
                          << ", Compiler error: Strings can only be assigned "
                             "to variables of the type \"CharacterPointer\", I "
                             "hope it's clear why. Aborting the compilation!"
                          << std::endl;
                exit(1);
              }
              if (!context.globalVariables.count(assignment.text)) {
                std::cerr << "Line " << assignment.lineNumber << ", Column "
                          << assignment.columnNumber
                          << ", Internal compiler error: Memory for the string "
                          << assignment.text
                          << " hasn't been allocated before a pointer to it is "
                             "being compiled. Aborting the compilation before "
                             "std::map throws exception."
                          << std::endl;
                exit(1);
              }
              globalDeclarations +=
                  "\t(data 0 (i32.const " +
                  std::to_string(context.globalVariables[variableName.text]) +
                  ") " +
                  getCharVectorRepresentationOfInteger32(
                      context.globalVariables.at(assignment.text)) +
                  ") ;;Hex of " +
                  std::to_string(context.globalVariables.at(assignment.text)) +
                  "\n";
            } else { // Simple assignment.
              if (childNode.text == "Character")
                globalDeclarations +=
                    "\t(data 0 (i32.const " +
                    std::to_string(context.globalVariables[variableName.text]) +
                    ") " +
                    getCharVectorRepresentationOfCharacter(
                        assignment.interpretAsACompileTimeIntegerConstant()) +
                    ")\n";
              else if (childNode.text == "Integer16")
                globalDeclarations +=
                    "\t(data 0 (i32.const " +
                    std::to_string(context.globalVariables[variableName.text]) +
                    ") " +
                    getCharVectorRepresentationOfInteger16(
                        assignment.interpretAsACompileTimeIntegerConstant()) +
                    ")\n";
              else if (childNode.text == "Integer32")
                globalDeclarations +=
                    "\t(data 0 (i32.const " +
                    std::to_string(context.globalVariables[variableName.text]) +
                    ") " +
                    getCharVectorRepresentationOfInteger32(
                        assignment.interpretAsACompileTimeIntegerConstant()) +
                    ")\n";
              else if (childNode.text == "Integer64")
                globalDeclarations +=
                    "\t(data 0 (i32.const " +
                    std::to_string(context.globalVariables[variableName.text]) +
                    ") " +
                    getCharVectorRepresentationOfInteger64(
                        assignment.interpretAsACompileTimeIntegerConstant()) +
                    ")\n";
              else if (childNode.text == "Decimal32")
                globalDeclarations +=
                    "\t(data 0 (i32.const " +
                    std::to_string(context.globalVariables[variableName.text]) +
                    ") " +
                    getCharVectorRepresentationOfDecimal32(
                        assignment.interpretAsACompileTimeDecimalConstant()) +
                    ")\n";
              else if (childNode.text == "Decimal64")
                globalDeclarations +=
                    "\t(data 0 (i32.const " +
                    std::to_string(context.globalVariables[variableName.text]) +
                    ") " +
                    getCharVectorRepresentationOfDecimal64(
                        assignment.interpretAsACompileTimeDecimalConstant()) +
                    ")\n";
              else {
                std::cerr << "Line " << assignment.lineNumber << ", Column "
                          << assignment.columnNumber
                          << ", Internal compiler error: Compiler got into a "
                             "forbidden state!"
                          << std::endl;
                exit(1);
              }
            }
          }
        }
      } else if (childNode.text == "Function") {
        function functionDeclaration;
        CompilationContext contextOfThatFunction = context;
        if (childNode.children.size() != 3) {
          std::cerr << "Line " << childNode.lineNumber << ", Column "
                    << childNode.columnNumber
                    << ", Compiler error: The AST is corrupt, the \"Function\" "
                       "node has less than 3 children, quitting now (or else "
                       "the compiler will segfault)!"
                    << std::endl;
          exit(1);
        }
        functionDeclaration.name = childNode.children[0].text;
        functionDeclaration.returnType = childNode.children[1].children[0].text;
        for (TreeNode argument : childNode.children[0].children) {
          functionDeclaration.argumentNames.push_back(
              argument.children[0].text);
          functionDeclaration.argumentTypes.push_back(argument.text);
          contextOfThatFunction.variableTypes[argument.children[0].text] =
              argument.text;
          for (auto &pair : contextOfThatFunction
                                .localVariables) // The reference operator '&'
                                                 // is needed because... C++.
            pair.second +=
                basicDataTypeSizes[argument.text]; // Push all the variables
                                                   // further back on the stack.
          contextOfThatFunction.stackSizeOfThisFunction +=
              basicDataTypeSizes[argument.text];
          contextOfThatFunction.stackSizeOfThisScope =
              contextOfThatFunction.stackSizeOfThisFunction;
          contextOfThatFunction.localVariables[argument.children[0].text] = 0;
          if (argument.children[0]
                  .children.size()) // If there is a default value.
            functionDeclaration.defaultArgumentValues.push_back(
                argument.children[0]
                    .children[0]
                    .interpretAsACompileTimeDecimalConstant());
          else
            functionDeclaration.defaultArgumentValues.push_back(0);
        }
        context.functions.push_back(functionDeclaration);
        contextOfThatFunction.functions.push_back(functionDeclaration);
        if (childNode.children[2].text == "External") {
          globalDeclarations += "\t(import \"JavaScript\" \"" +
                                functionDeclaration.name.substr(
                                    0, functionDeclaration.name.size() - 1) +
                                "\" (func $" +
                                functionDeclaration.name.substr(
                                    0, functionDeclaration.name.size() - 1) +
                                " ";
          for (std::string argumentType : functionDeclaration.argumentTypes)
            globalDeclarations +=
                "(param " +
                stringRepresentationOfWebAssemblyType
                    [mappingOfAECTypesToWebAssemblyTypes[argumentType]] +
                ") ";
          if (functionDeclaration.returnType != "Nothing")
            globalDeclarations +=
                "(result " +
                stringRepresentationOfWebAssemblyType
                    [mappingOfAECTypesToWebAssemblyTypes[functionDeclaration
                                                             .returnType]] +
                ")";
          globalDeclarations += "))\n";
        } else if (childNode.children[2].text == "Does") {
          globalDeclarations += "\t(func $" +
                                functionDeclaration.name.substr(
                                    0, functionDeclaration.name.size() - 1) +
                                " ";
          for (std::string argumentType : functionDeclaration.argumentTypes)
            globalDeclarations +=
                "(param " +
                stringRepresentationOfWebAssemblyType
                    [mappingOfAECTypesToWebAssemblyTypes[argumentType]] +
                ") ";
          if (functionDeclaration.returnType != "Nothing")
            globalDeclarations +=
                "(result " +
                stringRepresentationOfWebAssemblyType
                    [mappingOfAECTypesToWebAssemblyTypes[functionDeclaration
                                                             .returnType]] +
                ")";
          globalDeclarations += "\n";
          globalDeclarations +=
              "\t\t(global.set $stack_pointer ;;Allocate the space for the "
              "arguments of that function on the system stack.\n\t\t(i32.add "
              "(global.get $stack_pointer) (i32.const " +
              std::to_string(contextOfThatFunction.stackSizeOfThisScope) +
              ")))\n";
          for (unsigned int i = 0; i < functionDeclaration.argumentNames.size();
               i++) {
            if (functionDeclaration.argumentTypes[i] == "Character")
              globalDeclarations += "\t\t(i32.store8 ";
            else if (functionDeclaration.argumentTypes[i] == "Integer16")
              globalDeclarations += "\t\t(i32.store16 ";
            else if (functionDeclaration.argumentTypes[i] == "Integer32" or
                     std::regex_search(functionDeclaration.argumentTypes[i],
                                       std::regex("Pointer$")))
              globalDeclarations += "\t\t(i32.store ";
            else if (functionDeclaration.argumentTypes[i] == "Integer64")
              globalDeclarations += "\t\t(i64.store ";
            else if (functionDeclaration.argumentTypes[i] == "Decimal32")
              globalDeclarations += "\t\t(f32.store ";
            else if (functionDeclaration.argumentTypes[i] == "Decimal64")
              globalDeclarations += "\t\t(f64.store ";
            else {
              std::cerr << "Line " << childNode.children[2].lineNumber
                        << ", Column " << childNode.children[2].columnNumber
                        << ", Internal compiler error: The compiler got into a "
                           "forbidden state, quitting now so that the compiler "
                           "doesn't segfault!"
                        << std::endl;
              exit(1);
            }
            globalDeclarations +=
                "\n" +
                std::string(TreeNode(functionDeclaration.argumentNames[i],
                                     childNode.children[2].lineNumber,
                                     childNode.children[2].columnNumber)
                                .compileAPointer(contextOfThatFunction)
                                .indentBy(2)) +
                "\n\t\t\t(local.get " + std::to_string(i) + "))\n";
          }
          globalDeclarations +=
              childNode.children[2].compile(contextOfThatFunction).indentBy(1);
          globalDeclarations += ")\n\t(export \"" +
                                functionDeclaration.name.substr(
                                    0, functionDeclaration.name.size() - 1) +
                                "\" (func $" +
                                functionDeclaration.name.substr(
                                    0, functionDeclaration.name.size() - 1) +
                                "))\n";
        } else {
          std::cerr << "Line " << childNode.lineNumber << ", Column "
                    << childNode.columnNumber
                    << ", Internal compiler error: The compiler got into a "
                       "forbidden state, quitting now before segfaulting!"
                    << std::endl;
          exit(1);
        }
      } else {
        std::cerr << "Line " << childNode.lineNumber << ", Column "
                  << childNode.columnNumber
                  << ", Compiler error: No rule for compiling the token \""
                  << childNode.text << "\", aborting the compilation!"
                  << std::endl;
        exit(1);
      }
    }
    globalDeclarations += ")"; // End of the "module" S-expression.
    return AssemblyCode(globalDeclarations);
  }
  AssemblyCode compile(CompilationContext context) {
    std::cerr << "Internal compiler error: Some part of the compiler attempted "
                 "to recursively call the compiler from the beginning (leading "
                 "to an infinite loop), quitting now!"
              << std::endl;
    exit(1);
    return AssemblyCode(
        "()"); // So that the compiler doesn't throw a bunch of warnings about
               // the control reaching the end of a non-void function.
  }
  AssemblyCode compileAPointer(CompilationContext context) {
    std::cerr << "Internal compiler error: Some part of the compiler attempted "
                 "to get the assembly of the pointer of a module, which "
                 "doesn't make sense. Quitting now!"
              << std::endl;
    exit(1);
    return AssemblyCode("()");
  }
  int interpretAsACompileTimeIntegerConstant() {
    std::cerr
        << "Some part of the compiler attempted to convert a module to the "
           "compile time constant, which makes no sense. Quitting now!"
        << std::endl;
    exit(1);
    return 0;
  }
};
