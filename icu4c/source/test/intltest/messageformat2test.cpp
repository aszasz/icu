// © 2024 and later: Unicode, Inc. and others.

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#if !UCONFIG_NO_MF2

#include "unicode/calendar.h"
#include "messageformat2test.h"

using namespace icu::message2;

/*
  TODO: Tests need to be unified in a single format that
  both ICU4C and ICU4J can use, rather than being embedded in code.

  Tests are included in their current state to give a sense of
  how much test coverage has been achieved. Most of the testing is
  of the parser/serializer; the formatter needs to be tested more
  thoroughly.
*/

void
TestMessageFormat2::runIndexedTest(int32_t index, UBool exec,
                                  const char* &name, char* /*par*/) {
    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(testAPICustomFunctions);
    TESTCASE_AUTO(messageFormat1Tests);
    TESTCASE_AUTO(featureTests);
    TESTCASE_AUTO(testCustomFunctions);
    TESTCASE_AUTO(testBuiltInFunctions);
    TESTCASE_AUTO(testAPI);
    TESTCASE_AUTO(testAPISimple);
    TESTCASE_AUTO(testDataModelAPI);
    TESTCASE_AUTO(dataDrivenTests);
    TESTCASE_AUTO_END;
}

// Needs more tests
void TestMessageFormat2::testDataModelAPI() {
    IcuTestErrorCode errorCode1(*this, "testAPI");
    UErrorCode errorCode = (UErrorCode) errorCode1;

    using Pattern = data_model::Pattern;

    Pattern::Builder builder(errorCode);

    builder.add("a", errorCode);
    builder.add("b", errorCode);
    builder.add("c", errorCode);

    Pattern p = builder.build(errorCode);
    int32_t i = 0;
    for (auto iter = p.begin(); iter != p.end(); ++iter) {
        std::variant<UnicodeString, Expression, Markup> part = *iter;
        UnicodeString val = *std::get_if<UnicodeString>(&part);
        if (i == 0) {
            assertEquals("testDataModelAPI", val, "a");
        } else if (i == 1) {
            assertEquals("testDataModelAPI", val, "b");
        } else if (i == 2) {
            assertEquals("testDataModelAPI", val, "c");
        }
        i++;
    }
    assertEquals("testDataModelAPI", i, 3);
}

// Example for design doc -- version without null and error checks
void TestMessageFormat2::testAPISimple() {
    IcuTestErrorCode errorCode1(*this, "testAPI");
    UErrorCode errorCode = (UErrorCode) errorCode1;
    UParseError parseError;
    Locale locale = "en_US";

    // Since this is the example used in the
    // design doc, it elides null checks and error checks.
    // To be used in the test suite, it should include those checks
    // Null checks and error checks elided
    MessageFormatter::Builder builder(errorCode);
    MessageFormatter mf = builder.setPattern(u"Hello, {$userName}!", parseError, errorCode)
        .build(errorCode);

    std::map<UnicodeString, message2::Formattable> argsBuilder;
    argsBuilder["userName"] = message2::Formattable("John");
    MessageArguments args(argsBuilder, errorCode);

    UnicodeString result;
    result = mf.formatToString(args, errorCode);
    assertEquals("testAPI", result, "Hello, John!");

    mf = builder.setPattern("Today is {$today :date style=full}.", parseError, errorCode)
        .setLocale(locale)
        .build(errorCode);

    Calendar* cal(Calendar::createInstance(errorCode)); 
   // Sunday, October 28, 2136 8:39:12 AM PST
    cal->set(2136, Calendar::OCTOBER, 28, 8, 39, 12);
    UDate date = cal->getTime(errorCode);

    argsBuilder.clear();
    argsBuilder["today"] = message2::Formattable::forDate(date);
    args = MessageArguments(argsBuilder, errorCode);
    result = mf.formatToString(args, errorCode);
    assertEquals("testAPI", "Today is Sunday, October 28, 2136.", result);

    argsBuilder.clear();
    argsBuilder["photoCount"] = message2::Formattable((int64_t) 12);
    argsBuilder["userGender"] = message2::Formattable("feminine");
    argsBuilder["userName"] = message2::Formattable("Maria");
    args = MessageArguments(argsBuilder, errorCode);

    mf = builder.setPattern(".match {$photoCount :number} {$userGender :string}\n\
                      1 masculine {{{$userName} added a new photo to his album.}}\n \
                      1 feminine {{{$userName} added a new photo to her album.}}\n \
                      1 * {{{$userName} added a new photo to their album.}}\n \
                      * masculine {{{$userName} added {$photoCount} photos to his album.}}\n \
                      * feminine {{{$userName} added {$photoCount} photos to her album.}}\n \
                      * * {{{$userName} added {$photoCount} photos to their album.}}", parseError, errorCode)
        .setLocale(locale)
        .build(errorCode);
    result = mf.formatToString(args, errorCode);
    assertEquals("testAPI", "Maria added 12 photos to her album.", result);

    delete cal;
}

// Design doc example, with more details
void TestMessageFormat2::testAPI() {
    IcuTestErrorCode errorCode(*this, "testAPI");
    TestCase::Builder testBuilder;

    // Pattern: "Hello, {$userName}!"
    TestCase test(testBuilder.setName("testAPI")
                  .setPattern("Hello, {$userName}!")
                  .setArgument("userName", "John")
                  .setExpected("Hello, John!")
                  .setLocale("en_US")
                  .build());
    TestUtils::runTestCase(*this, test, errorCode);

    // Pattern: "{Today is {$today ..."
    LocalPointer<Calendar> cal(Calendar::createInstance(errorCode));
    // Sunday, October 28, 2136 8:39:12 AM PST
    cal->set(2136, Calendar::OCTOBER, 28, 8, 39, 12);
    UDate date = cal->getTime(errorCode);

    test = testBuilder.setName("testAPI")
        .setPattern("Today is {$today :date style=full}.")
        .setDateArgument("today", date)
        .setExpected("Today is Sunday, October 28, 2136.")
        .setLocale("en_US")
        .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // Pattern matching - plural
    UnicodeString pattern = ".match {$photoCount :string} {$userGender :string}\n\
                      1 masculine {{{$userName} added a new photo to his album.}}\n \
                      1 feminine {{{$userName} added a new photo to her album.}}\n \
                      1 * {{{$userName} added a new photo to their album.}}\n \
                      * masculine {{{$userName} added {$photoCount} photos to his album.}}\n \
                      * feminine {{{$userName} added {$photoCount} photos to her album.}}\n \
                      * * {{{$userName} added {$photoCount} photos to their album.}}";


    int64_t photoCount = 12;
    test = testBuilder.setName("testAPI")
        .setPattern(pattern)
        .setArgument("photoCount", photoCount)
        .setArgument("userGender", "feminine")
        .setArgument("userName", "Maria")
        .setExpected("Maria added 12 photos to her album.")
        .setLocale("en_US")
        .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // Built-in functions
    pattern = ".match {$photoCount :number} {$userGender :string}\n\
                      1 masculine {{{$userName} added a new photo to his album.}}\n \
                      1 feminine {{{$userName} added a new photo to her album.}}\n \
                      1 * {{{$userName} added a new photo to their album.}}\n \
                      * masculine {{{$userName} added {$photoCount} photos to his album.}}\n \
                      * feminine {{{$userName} added {$photoCount} photos to her album.}}\n \
                      * * {{{$userName} added {$photoCount} photos to their album.}}";

    photoCount = 1;
    test = testBuilder.setName("testAPI")
        .setPattern(pattern)
        .setArgument("photoCount", photoCount)
        .setArgument("userGender", "feminine")
        .setArgument("userName", "Maria")
        .setExpected("Maria added a new photo to her album.")
        .setLocale("en_US")
        .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

// Custom functions example from the ICU4C API design doc
// Note: error/null checks are omitted
void TestMessageFormat2::testAPICustomFunctions() {
    IcuTestErrorCode errorCode1(*this, "testAPICustomFunctions");
    UErrorCode errorCode = (UErrorCode) errorCode1;
    UParseError parseError;
    Locale locale = "en_US";

    // Set up custom function registry
    MFFunctionRegistry::Builder builder(errorCode);
    MFFunctionRegistry functionRegistry =
        builder.adoptFormatter(data_model::FunctionName("person"), new PersonNameFormatterFactory(), errorCode)
               .build();

    Person* person = new Person(UnicodeString("Mr."), UnicodeString("John"), UnicodeString("Doe"));

    std::map<UnicodeString, message2::Formattable> argsBuilder;
    argsBuilder["name"] = message2::Formattable(person);
    MessageArguments arguments(argsBuilder, errorCode);

    MessageFormatter::Builder mfBuilder(errorCode);
    UnicodeString result;
    // This fails, because we did not provide a function registry:
    MessageFormatter mf = mfBuilder.setPattern("Hello {$name :person formality=informal}", parseError, errorCode)
                                    .setLocale(locale)
                                    .build(errorCode);
    result = mf.formatToString(arguments, errorCode);
    assertEquals("testAPICustomFunctions", U_MF_UNKNOWN_FUNCTION_ERROR, errorCode);

    errorCode = U_ZERO_ERROR;
    mfBuilder.setFunctionRegistry(functionRegistry).setLocale(locale);

    mf = mfBuilder.setPattern("Hello {$name :person formality=informal}", parseError, errorCode)
                    .build(errorCode);
    result = mf.formatToString(arguments, errorCode);
    assertEquals("testAPICustomFunctions", "Hello John", result);

    mf = mfBuilder.setPattern("Hello {$name :person formality=formal}", parseError, errorCode)
                    .build(errorCode);
    result = mf.formatToString(arguments, errorCode);
    assertEquals("testAPICustomFunctions", "Hello Mr. Doe", result);

    mf = mfBuilder.setPattern("Hello {$name :person formality=formal length=long}", parseError, errorCode)
                    .build(errorCode);
    result = mf.formatToString(arguments, errorCode);
    assertEquals("testAPICustomFunctions", "Hello Mr. John Doe", result);

    // By type
    MFFunctionRegistry::Builder builderByType(errorCode);
    FunctionName personFormatterName("person");
    MFFunctionRegistry functionRegistryByType =
        builderByType.adoptFormatter(personFormatterName,
                                   new PersonNameFormatterFactory(),
                                   errorCode)
                     .setDefaultFormatterNameByType("person",
                                                    personFormatterName,
                                                    errorCode)
                     .build();
    mfBuilder.setFunctionRegistry(functionRegistryByType);
    mf = mfBuilder.setPattern("Hello {$name}", parseError, errorCode)
        .setLocale(locale)
        .build(errorCode);
    result = mf.formatToString(arguments, errorCode);
    assertEquals("testAPICustomFunctions", U_ZERO_ERROR, errorCode);
    // Expect "Hello John" because in the custom function we registered,
    // "informal" is the default formality and "length" is the default length
    assertEquals("testAPICustomFunctions", "Hello John", result);
    delete person;
}

void TestMessageFormat2::dataDrivenTests() {
    IcuTestErrorCode errorCode(*this, "jsonTests");

    jsonTestsFromFiles(errorCode);
}

#endif /* #if !UCONFIG_NO_MF2 */

#endif /* #if !UCONFIG_NO_FORMATTING */

