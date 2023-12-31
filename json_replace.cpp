/**
 * This file contains the function json_replace, which gets a string or
 * C-string and returns a new string where each key that ends with
 * TARGET_SUFFIX will have its corresponding value replaced with REPLACE_SUFFIX.
 *
 * Current functionality only works with values that are strings or arrays of
 * strings. This does not account for numbers, booleans, nested jsons, or nested brackets. Using
 * this function with other json values or invalid json causes undefined behavior, such as:
 *     segmentation fault,
 *     throwing std::invalid_argument,
 *     returning a partial json,
 *     not replacing values.
 *
 *
 * Ex. 1) "key_X" : "value" --> "key_X" : "*"
 *     2) "key_X" : ["val1", "val2"] --> "key_X" : ["*", "*"]
 */

#include <iostream>
#include <string>
#include <cassert>
#include <cstring>

#define TARGET_SUFFIX "_X"
#define REPLACE_CHAR '*'
#define ESCAPED_TARGET_SUFFIX TARGET_SUFFIX"\""
#define JSON_READ_ERROR_MSG "json_replace only allows for strings and "\
"array of strings as json values."

/**
 * Reads whitespace from the C-string into new C-string.
 * Terminating character ('\0') is not counted as whitespace, and is not read.
 * @param p_src pointer to original C-string
 * @param p_dest pointer to new C-string
 */
void read_json_filler (const char **p_src, char **p_dest)
{
  while (**p_src != '\0' && **p_src != '"' && **p_src != '[')
  {
    *((*p_dest)++) = *((*p_src)++);
  }
}

/**
 * Reads a quoted string from the C-string, and writes the replace value into the new string.
 * If there is nothing between the quotes, no replacing will be done
 * @param p_src pointer to original C-string
 * @param p_dest pointer to new C-string
 */
void read_and_replace_json_string (const char **p_src, char **p_dest)
{
  *((*p_dest)++) = *((*p_src)++);
  bool escaped = false;
  // will not replace an empty string
  if (**p_src == '"')
  {
    *((*p_dest)++) = *((*p_src)++);
    return;
  }
  // read body of string. allows for reading internal escaped quotes.
  while (**p_src != '"' || escaped)
  {
    escaped = false;
    if (**p_src == '\\')
      escaped = true;
    ++(*p_src);
  }

  *((*p_dest)++) = REPLACE_CHAR;
  *((*p_dest)++) = *((*p_src)++);
}

/**
 * Reads a json string from a C-string, starting and ending with quotes, into a new C-string.
 * @param p_src pointer to original C-string
 * @param p_dest pointer to new C-string
 */
void read_json_string (const char **p_src, char **p_dest)
{
  *((*p_dest)++) = *((*p_src)++);
  bool escaped = false;
  // read body of string. allows for reading internal escaped quotes.
  while (**p_src != '"' || escaped)
  {
    escaped = false;
    if (**p_src == '\\')
      escaped = true;
    *((*p_dest)++) = *((*p_src)++);
  }

  *((*p_dest)++) = *((*p_src)++);
}

/**
 * Reads a json value from the string, including "[] characters, into new string.
 * C-strings will be left pointing to '\0', ',', or right after '"', ']'.
 * Only strings or arrays of strings can be read.
 *
 * @param p_src pointer to original C-string
 * @param p_dest pointer to new C-string
 * @param replace whether or not to replace the value or values
 */
void read_json_value (const char **p_src, char **p_dest, bool replace) noexcept (false)
{
  if (**p_src == '"')
  {
    if (replace)
      read_and_replace_json_string (p_src, p_dest);
    else
      read_json_string (p_src, p_dest);
    return;
  }

  if (**p_src == '[')
  {
    *((*p_dest)++) = *((*p_src)++);
    while (**p_src != ']')
    {
      if (**p_src == '"')
      {
        if (replace)
          read_and_replace_json_string (p_src, p_dest);
        else
          read_json_string (p_src, p_dest);
        continue;
      }
      *((*p_dest)++) = *((*p_src)++);
    }
    *((*p_dest)++) = *((*p_src)++);
    return;
  }

  throw std::invalid_argument (JSON_READ_ERROR_MSG);
}

/**
 * Reads through the json and creates a copy of it where all keys that end with TARGET_SUFFIX
 * have their corresponding values replaced with REPLACE_CHAR. Only works for json values of
 * strings or arrays of strings.
 * @param src C-string to read from.
 * @return New string with replaced values.
 */
void json_replace_no_try_catch (const char *src, char *dest)
{
  // loop reads ("key" : "value" ,? ) each iteration
  size_t min_key_len = strlen (ESCAPED_TARGET_SUFFIX);
  while (*src != 0)
  {
    read_json_filler (&src, &dest);

    // read key
    char *temp = dest;
    read_json_string (&src, &dest);
    bool replace = dest - temp >= min_key_len &&
                   strncmp (dest - min_key_len, ESCAPED_TARGET_SUFFIX, min_key_len) == 0;

    // read ws:ws
    read_json_filler (&src, &dest);

    // read value
    read_json_value (&src, &dest, replace);

    // add '}' or comma
    read_json_filler (&src, &dest);
  }
  *dest = '\0';
}

/**
 * Reads through the json and creates a copy of it where all keys that end with TARGET_SUFFIX
 * have their corresponding values replaced with REPLACE_CHAR. Only works for json values of
 * strings or arrays of strings.
 * @param str C-string to read from.
 * @return New string with replaced values.
 */
std::string json_replace (const char *str) noexcept (false)
{
  try
  {
    char *dest = new char[strlen (str) + 1];
    json_replace_no_try_catch (str, dest);
    std::string new_str (dest);
    delete[] dest;
    return new_str;
  }
  catch (std::invalid_argument &e)
  {
    throw std::invalid_argument (JSON_READ_ERROR_MSG);
  }
}

/**
 * Reads through the json and creates a copy of it where all keys that end with TARGET_SUFFIX
 * have their corresponding values replaced with REPLACE_CHAR. Only works for json values of
 * strings or arrays of strings.
 * @param str string to read from.
 * @return New string with replaced values.
 */
std::string json_replace (const std::string &str) noexcept (false)
{
  return json_replace (str.c_str ());
}

void json_test_compare (int testNum, const std::string &testName, const std::string &expected,
                        const std::string &actual)
{
  std::cout << "Running test " << testNum << ": " << testName << " ..." << std::endl;
  if (expected != actual)
  {
    std::cout << "Expected: " << expected << "\nActual  : " << actual << std::endl;
  }
  assert(expected == actual);
  std::cout << "Test " << testNum << " passed: " << testName << std::endl;
}

void json_replace_tests ()
{
  std::cout << "running tests..." << std::endl;

  std::string input1 = R"("key" : "value" , "1":"2", "array0" : [], "arr1": ["hello"], "arr2":["1" , "2,"], "arr3:" : ["\"]"])";
  std::string expected1 = R"("key" : "value" , "1":"2", "array0" : [], "arr1": ["hello"], "arr2":["1" , "2,"], "arr3:" : ["\"]"])";
  json_test_compare (1, "various inputs", expected1, json_replace (input1));

  std::string input2 = R"("key" : "value", "k2_X": "value2", "k3_X": ["abc"], "k4_X": ["a", "b","c"])";
  std::string expected2 = R"("key" : "value", "k2_X": "*", "k3_X": ["*"], "k4_X": ["*", "*","*"])";
  json_test_compare (2, "various replacements", expected2, json_replace (input2));

  std::string input3 = R"("key" : "value", "k2_X": "value2", "k3_X": ["abc"], "k4_X": ["a", "b","c"])";
  json_test_compare (3, "original string unchanged", input3, input2);

  std::string input4 = R"("key" : "va\"l[ue]" ,
"k\"1[e]:,y":["[h,i]", "12:{}\"", ""]   ,   "{\] b0_X" : "v\a\"l[ue]", "key_X": ["[h,i]", "12:{}\"", ""])";
  std::string expected4 = R"("key" : "va\"l[ue]" ,
"k\"1[e]:,y":["[h,i]", "12:{}\"", ""]   ,   "{\] b0_X" : "*", "key_X": ["*", "*", ""])";
  json_test_compare (4, "various edge cases", expected4, json_replace (input4));

  std::string input5 = R"("key" : "אב", "key2" : "[עברית]", "key3_X" : "עברית", "key4_X":["א"])";
  std::string expected5 = R"("key" : "אב", "key2" : "[עברית]", "key3_X" : "*", "key4_X":["*"])";
  json_test_compare (5, "hebrew", expected5, json_replace (input5));

  std::string input6 = R"({"key" : "val", "key_X" : "val"})";
  std::string expected6 = R"({"key" : "val", "key_X" : "*"})";
  json_test_compare (6, "curly bracketed json", expected6, json_replace (input6));

  std::string input7 = R"("key1": " 形式 ", "key2":[" 形式 "], "key3_X": " 形式 ", "key4_X" : [" 形式 "])";
  std::string expected7 = R"("key1": " 形式 ", "key2":[" 形式 "], "key3_X": "*", "key4_X" : ["*"])";
  json_test_compare (7, "japanese", expected7, json_replace (input7));

  std::string input8 = R"("k":"val", "_X": "val2", "": "")";
  std::string expected8 = R"("k":"val", "_X": "*", "": "")";
  json_test_compare (8, "short keys", expected8, json_replace (input8));

  std::cout << "Passed all tests!" << std::endl;
}

int main ()
{
  json_replace_tests ();
  return EXIT_SUCCESS;
}