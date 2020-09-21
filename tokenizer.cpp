#include "TreeNode.cpp"

bool isComposedOfAlnumsAndOneDot(
    std::string token) // Done often, so it's probably better (and more
                       // portable) to do it this way than in REGEX.
{
  bool passedOverADot = false;
  for (unsigned i = 0; i < token.size(); i++)
    if (token[i] == '.' and !passedOverADot and i != 0)
      passedOverADot = true;
    else if (token[i] == '.')
      return false;
    else if (!std::isalnum(token[i]) and token[i] != '_')
      return false;
  return true;
}

bool isAllWhitespace(
    std::string token) // No obvious way to do it in REGEX so that CLANG on
                       // Linux won't miscompile it.
{
  for (unsigned i = 0; i < token.size(); i++)
    if (!std::isspace(token[i]))
      return false;
  return true;
}

bool isWordCharacterButNotDigit(
    char c) // No obvious way to do it in REGEX so that CLANG on Linux won't
            // miscompile it.
{
  return (std::isalnum(c) or c == '_') and !std::isdigit(c);
}

std::vector<TreeNode> TreeNode::tokenize(std::string input) {
#ifndef NDEBUG
  std::cerr << "DEBUG: Tokenizing the string \"" << input << "\"..."
            << std::endl;
#endif
  using std::regex;
  using std::regex_match;
  using std::string;
  using std::to_string;
  auto tokenizedExpression = std::vector<TreeNode>();
  int currentLine = 1, currentColumn = 1;
  bool areWeInAString = false, areWeInAComment = false;
  string stringDelimiter, commentDelimiter;
  for (unsigned int i = 0; i < input.size(); i++) {
#ifndef NDEBUG
    std::cerr << "DEBUG: Now we are tokenizing the character #" << i << ": '"
              << input[i] << "'." << std::endl;
    if (areWeInAString)
      std::cerr << "DEBUG: We are in a string." << std::endl;
    if (areWeInAComment)
      std::cerr << "DEBUG: We are in a comment." << std::endl;
#endif
    if (tokenizedExpression.size() == 0) {
      if (input.size() - i > 2 and
          (input.substr(i, 2) == "//" or input.substr(i, 2) == "/*")) {
        areWeInAComment = true;
        commentDelimiter = input.substr(i, 2);
        tokenizedExpression.push_back(TreeNode(string(), 1, 1));
        currentColumn++;
        continue;
      }
      tokenizedExpression.push_back(TreeNode(input.substr(i, 1), 1, 1));
      currentColumn++;
      if (input[i] == '"' or input[i] == '\'') {
        areWeInAString = true;
        stringDelimiter = input.substr(i, 1);
      }
    } else if ((input[i] == '"' or input[i] == '\'') and !areWeInAComment) {
      if (areWeInAString and stringDelimiter == input.substr(i, 1) and
          (i == 0 or input[i - 1] != '\\')) {
        tokenizedExpression.back().text.push_back(input[i]);
        tokenizedExpression.push_back(
            TreeNode(string(), currentLine, currentColumn));
        currentColumn++;
        areWeInAString = false;
      } else if (!areWeInAString) {
        currentColumn++;
        areWeInAString = true;
        stringDelimiter = input.substr(i, 1);
        tokenizedExpression.push_back(
            TreeNode(input.substr(i, 1), currentLine, currentColumn));
      } else
        tokenizedExpression.back().text.push_back(input[i]);
    } else if (input.size() - i > 2 and
               (input.substr(i, 2) == "//" or input.substr(i, 2) == "/*") and
               !areWeInAString and !areWeInAComment) {
      areWeInAComment = true;
      commentDelimiter = input.substr(i, 2);
    } else if (input[i] == '\n') { // If we came to the end of a line.
      if (areWeInAString) {
        std::cerr << "Line " << currentLine << ", Column "
                  << tokenizedExpression.back().columnNumber
                  << ", Tokenizer error: Unterminated string literal!"
                  << std::endl;
        areWeInAString = false;
      }
      if (areWeInAComment and commentDelimiter == "//")
        areWeInAComment = false;
      else if (areWeInAComment) {
        currentLine++;
        currentColumn = 1;
        continue;
      }
      currentLine++;
      currentColumn = 1;
      tokenizedExpression.push_back(
          TreeNode(string(), currentLine, currentColumn));
    } else if (isspace(input[i]) and !areWeInAString and
               !areWeInAComment) { // If we came to some other whitespace.
      currentColumn++;
      tokenizedExpression.push_back(
          TreeNode(string(), currentLine, currentColumn));
    } else if ((std::isalnum(input[i]) or input[i] == '_') and
               isComposedOfAlnumsAndOneDot(tokenizedExpression.back().text) and
               !areWeInAString and !areWeInAComment) // Names and numbers
    {
      currentColumn++;
      tokenizedExpression.back().text += input[i];
    } else if (input[i] == '.' and
               regex_match(
                   tokenizedExpression.back().text,
                   regex("[0-9]+",
                         std::regex::extended)) and // Some C++ runtime
                                                    // environments apparently
                                                    // think "[0-9]+" wouldn't
                                                    // be a proper REGEX in
                                                    // JavaScript (it would be).
               !areWeInAString and
               !areWeInAComment) // If we are currently
                                 // tokenizing a number, a dot
                                 // character is a decimal point.
    {
      currentColumn++;
      tokenizedExpression.back().text += input[i];
    } else if (!areWeInAString and !areWeInAComment) {
      currentColumn++;
      tokenizedExpression.push_back(
          TreeNode(input.substr(i, 1), currentLine, currentColumn));
    } else if (areWeInAString and !areWeInAComment) {
      currentColumn++;
      tokenizedExpression.back().text += input[i];
    } else if (areWeInAComment and commentDelimiter == "/*" and
               input.size() - i > 2 and input.substr(i, 2) == "*/" and
               !areWeInAString) {
      areWeInAComment = false;
      currentColumn += 2;
      tokenizedExpression.push_back(
          TreeNode(string(), currentLine, currentColumn));
      i++;
    } else if (!areWeInAString and areWeInAComment) {
      currentColumn++;
    } else {
      std::cerr << "Line " << currentLine << ", Column " << currentColumn
                << ", Internal compiler error: Tokenizer is in the "
                   "forbidden state!"
                << std::endl;
    }
  }
  for (auto iterator = tokenizedExpression.begin();
       iterator < tokenizedExpression.end(); iterator++)
    if (iterator->text.size() == 3 and iterator->text.substr(0, 1) == "'" and
        iterator->text.substr(2, 1) == "'")
      iterator->text = std::to_string(int(iterator->text[1]));
  for (auto iterator = tokenizedExpression.begin();
       iterator < tokenizedExpression.end();
       iterator++) // Turn escape sequences into numbers.
    if (iterator->text == "'\\n'")
      iterator->text = to_string(int('\n'));
    else if (iterator->text == "'\\t'")
      iterator->text = to_string(int('\t'));
    else if (iterator->text == "'\\''")
      iterator->text = to_string(int('\''));
    else if (iterator->text == "'\\\\'")
      iterator->text = to_string(int('\\'));
  for (auto iterator = tokenizedExpression.begin();
       iterator < tokenizedExpression.end(); iterator++)
    if (isAllWhitespace(iterator->text)) // Delete empty nodes.
    {
      iterator--;
      tokenizedExpression.erase(iterator + 1);
    }
  for (unsigned int i = 1; i < tokenizedExpression.size(); i++)
    if ((tokenizedExpression[i].text == "(" or // Mark the names of functions...
         tokenizedExpression[i].text ==
             "[") and //...and arrays with ending '(' or '['
        isWordCharacterButNotDigit(
            tokenizedExpression[i - 1].text[0]) //...for the parser.
        and tokenizedExpression[i - 1].text != "If" and
        tokenizedExpression[i - 1].text != "ElseIf" and
        tokenizedExpression[i - 1].text != "While" and
        tokenizedExpression[i - 1].text != "Return" and
        tokenizedExpression[i - 1].text != "and" and
        tokenizedExpression[i - 1].text != "or") {
      tokenizedExpression[i - 1].text += tokenizedExpression[i].text;
      tokenizedExpression.erase(tokenizedExpression.begin() + i);
    }
  for (unsigned int i = 1; i < tokenizedExpression.size(); i++)
    if (tokenizedExpression[i].text == "=" and
        (tokenizedExpression[i - 1].text == ":" // The ":=" assignment operator.
         or
         tokenizedExpression[i - 1].text == "+" // The "+=" assignment operator.
         or tokenizedExpression[i - 1].text == "-" or
         tokenizedExpression[i - 1].text == "*" or
         tokenizedExpression[i - 1].text == "/")) {
      tokenizedExpression[i - 1].text = tokenizedExpression[i - 1].text + "=";
      tokenizedExpression.erase(tokenizedExpression.begin() + i);
    }
  for (unsigned int i = 1; i < tokenizedExpression.size(); i++)
    if (tokenizedExpression[i].text[0] == '"' and
        tokenizedExpression[i - 1].text[0] ==
            '"') // Concatenate two strings next to each other (as in C and
                 // C++).
    {
      tokenizedExpression[i - 1].text =
          tokenizedExpression[i - 1].text.substr(
              0, tokenizedExpression[i - 1].text.size() - 1) +
          tokenizedExpression[i].text.substr(1);
      tokenizedExpression.erase(tokenizedExpression.begin() + i);
      i--;
    }
  for (unsigned int i = 1; i < tokenizedExpression.size(); i++)
    if (tokenizedExpression[i].text == "{" // Hints to the code formatter,
                                           // will make parsing harder
                                           // if passed on to it.
        and (tokenizedExpression[i - 1].text == "Does" or
             tokenizedExpression[i - 1].text == "Then" or
             tokenizedExpression[i - 1].text == "Loop" or
             tokenizedExpression[i - 1].text == "Of" or
             tokenizedExpression[i - 1].text == "Else")) {
      tokenizedExpression.erase(tokenizedExpression.begin() + i);
      i--;
    }
  if (!tokenizedExpression.empty()) {
    for (unsigned int i = 0; i < tokenizedExpression.size() - 1; i++)
      if (tokenizedExpression[i].text == "}" and
          (tokenizedExpression[i + 1].text == "EndFunction" or
           tokenizedExpression[i + 1].text == "Else" or
           tokenizedExpression[i + 1].text == "ElseIf" or
           tokenizedExpression[i + 1].text == "EndIf" or
           tokenizedExpression[i + 1].text == "EndWhile" or
           tokenizedExpression[i + 1].text == "EndStructure")) {
        tokenizedExpression.erase(tokenizedExpression.begin() + i);
        i--;
      }
  }
#ifndef NDEBUG
  std::cerr << "DEBUG: Finished tokenizing the string \"" << input << "\"."
            << std::endl;
#endif
  return tokenizedExpression;
}
