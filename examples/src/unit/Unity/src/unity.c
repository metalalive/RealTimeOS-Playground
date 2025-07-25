/* =========================================================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007-14 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
============================================================================ */

#include "unity_internals.h"
#define UNITY_INCLUDE_SETUP_STUBS
#include "unity.h"
#include <stddef.h>

/* If omitted from header, declare overrideable prototypes here so they're ready for use */
#ifdef UNITY_OMIT_OUTPUT_CHAR_HEADER_DECLARATION
void UNITY_OUTPUT_CHAR(int);
#endif

/* Helpful macros for us to use here in Assert functions */
#define UNITY_FAIL_AND_BAIL   { Unity.CurrentTestFailed  = 1; UNITY_OUTPUT_FLUSH(); TEST_ABORT(); }
#define UNITY_IGNORE_AND_BAIL { Unity.CurrentTestIgnored = 1; UNITY_OUTPUT_FLUSH(); TEST_ABORT(); }
#define RETURN_IF_FAIL_OR_IGNORE if (Unity.CurrentTestFailed || Unity.CurrentTestIgnored) return

struct UNITY_STORAGE_T  Unity = {0};

#ifdef UNITY_OUTPUT_COLOR
static const char UnityStrOk[]                     = "\033[42mOK\033[00m";
static const char UnityStrPass[]                   = "\033[42mPASS\033[00m";
static const char UnityStrFail[]                   = "\033[41mFAIL\033[00m";
static const char UnityStrIgnore[]                 = "\033[43mIGNORE\033[00m";
#else
static const char UnityStrOk[]                     = "OK";
static const char UnityStrPass[]                   = "PASS";
static const char UnityStrFail[]                   = "FAIL";
static const char UnityStrIgnore[]                 = "IGNORE";
#endif
const char UnityStrErrFloat[]                      = "Unity Floating Point Disabled";
const char UnityStrErrDouble[]                     = "Unity Double Precision Disabled";
const char UnityStrErr64[]                         = "Unity 64-bit Support Disabled";

/*-----------------------------------------------
 * Pretty Printers & Test Result Output Handlers
 *-----------------------------------------------*/

/*-----------------------------------------------*/
/* Local helper function to print characters. */
static void UnityPrintChar(const char* pch)
{
    /* printable characters plus CR & LF are printed */
    if ((*pch <= 126) && (*pch >= 32))
    {
        UNITY_OUTPUT_CHAR(*pch);
    }
    /* write escaped carriage returns */
    else if (*pch == 13)
    {
        UNITY_OUTPUT_CHAR('\\');
        UNITY_OUTPUT_CHAR('r');
    }
    /* write escaped line feeds */
    else if (*pch == 10)
    {
        UNITY_OUTPUT_CHAR('\\');
        UNITY_OUTPUT_CHAR('n');
    }
    /* unprintable characters are shown as codes */
    else
    {
        UNITY_OUTPUT_CHAR('\\');
        UNITY_OUTPUT_CHAR('x');
        UnityPrintNumberHex((UNITY_UINT)*pch, 2);
    }
}

/*-----------------------------------------------*/
/* Local helper function to print ANSI escape strings e.g. "\033[42m". */
#ifdef UNITY_OUTPUT_COLOR
static UNITY_UINT UnityPrintAnsiEscapeString(const char* string)
{
    const char* pch = string;
    UNITY_UINT count = 0;

    while (*pch && (*pch != 'm'))
    {
        UNITY_OUTPUT_CHAR(*pch);
        pch++;
        count++;
    }
    UNITY_OUTPUT_CHAR('m');
    count++;

    return count;
}
#endif

/*-----------------------------------------------*/
void UnityPrint(const char* string)
{
    const char* pch = string;

    if (pch != NULL)
    {
        while (*pch)
        {
#ifdef UNITY_OUTPUT_COLOR
            /* print ANSI escape code */
            if ((*pch == 27) && (*(pch + 1) == '['))
            {
                pch += UnityPrintAnsiEscapeString(pch);
                continue;
            }
#endif
            UnityPrintChar(pch);
            pch++;
        }
    }
}

/*-----------------------------------------------*/
void UnityPrintLen(const char* string, const UNITY_UINT32 length)
{
    const char* pch = string;

    if (pch != NULL)
    {
        while (*pch && ((UNITY_UINT32)(pch - string) < length))
        {
            /* printable characters plus CR & LF are printed */
            if ((*pch <= 126) && (*pch >= 32))
            {
                UNITY_OUTPUT_CHAR(*pch);
            }
            /* write escaped carriage returns */
            else if (*pch == 13)
            {
                UNITY_OUTPUT_CHAR('\\');
                UNITY_OUTPUT_CHAR('r');
            }
            /* write escaped line feeds */
            else if (*pch == 10)
            {
                UNITY_OUTPUT_CHAR('\\');
                UNITY_OUTPUT_CHAR('n');
            }
            /* unprintable characters are shown as codes */
            else
            {
                UNITY_OUTPUT_CHAR('\\');
                UNITY_OUTPUT_CHAR('x');
                UnityPrintNumberHex((UNITY_UINT)*pch, 2);
            }
            pch++;
        }
    }
}

/*-----------------------------------------------*/
void UnityPrintNumberByStyle(const UNITY_INT number, const UNITY_DISPLAY_STYLE_T style)
{
    if ((style & UNITY_DISPLAY_RANGE_INT) == UNITY_DISPLAY_RANGE_INT)
    {
        UnityPrintNumber(number);
    }
    else if ((style & UNITY_DISPLAY_RANGE_UINT) == UNITY_DISPLAY_RANGE_UINT)
    {
        UnityPrintNumberUnsigned((UNITY_UINT)number);
    }
    else
    {
        UNITY_OUTPUT_CHAR('0');
        UNITY_OUTPUT_CHAR('x');
        UnityPrintNumberHex((UNITY_UINT)number, (char)((style & 0xF) * 2));
    }
}

/*-----------------------------------------------*/
void UnityPrintNumber(const UNITY_INT number_to_print)
{
    UNITY_UINT number = (UNITY_UINT)number_to_print;

    if (number_to_print < 0)
    {
        /* A negative number, including MIN negative */
        UNITY_OUTPUT_CHAR('-');
        number = -number;
    }
    UnityPrintNumberUnsigned(number);
}

/*-----------------------------------------------
 * basically do an itoa using as little ram as possible */
void UnityPrintNumberUnsigned(const UNITY_UINT number)
{
    UNITY_UINT divisor = 1;

    /* figure out initial divisor */
    while (number / divisor > 9)
    {
        divisor *= 10;
    }

    /* now mod and print, then divide divisor */
    do
    {
        UNITY_OUTPUT_CHAR((char)('0' + (number / divisor % 10)));
        divisor /= 10;
    } while (divisor > 0);
}

/*-----------------------------------------------*/
void UnityPrintNumberHex(const UNITY_UINT number, const char nibbles_to_print)
{
    int nibble;
    char nibbles = nibbles_to_print;

    if ((unsigned)nibbles > (2 * sizeof(number)))
    {
        nibbles = 2 * sizeof(number);
    }

    while (nibbles > 0)
    {
        nibbles--;
        nibble = (int)(number >> (nibbles * 4)) & 0x0F;
        if (nibble <= 9)
        {
            UNITY_OUTPUT_CHAR((char)('0' + nibble));
        }
        else
        {
            UNITY_OUTPUT_CHAR((char)('A' - 10 + nibble));
        }
    }
}

/*-----------------------------------------------*/
void UnityPrintMask(const UNITY_UINT mask, const UNITY_UINT number)
{
    UNITY_UINT current_bit = (UNITY_UINT)1 << (UNITY_INT_WIDTH - 1);
    UNITY_INT32 i;

    for (i = 0; i < UNITY_INT_WIDTH; i++)
    {
        if (current_bit & mask)
        {
            if (current_bit & number)
            {
                UNITY_OUTPUT_CHAR('1');
            }
            else
            {
                UNITY_OUTPUT_CHAR('0');
            }
        }
        else
        {
            UNITY_OUTPUT_CHAR('X');
        }
        current_bit = current_bit >> 1;
    }
}

/*-----------------------------------------------*/
#ifndef UNITY_EXCLUDE_FLOAT_PRINT
/*
 * This function prints a floating-point value in a format similar to
 * printf("%.7g") on a single-precision machine or printf("%.9g") on a
 * double-precision machine.  The 7th digit won't always be totally correct
 * in single-precision operation (for that level of accuracy, a more
 * complicated algorithm would be needed).
 */
void UnityPrintFloat(const UNITY_DOUBLE input_number)
{
#ifdef UNITY_INCLUDE_DOUBLE
    static const int sig_digits = 9;
    static const UNITY_INT32 min_scaled = 100000000;
    static const UNITY_INT32 max_scaled = 1000000000;
#else
    static const int sig_digits = 7;
    static const UNITY_INT32 min_scaled = 1000000;
    static const UNITY_INT32 max_scaled = 10000000;
#endif

    UNITY_DOUBLE number = input_number;

    /* print minus sign (including for negative zero) */
    if ((number < 0.0f) || ((number == 0.0f) && ((1.0f / number) < 0.0f)))
    {
        UNITY_OUTPUT_CHAR('-');
        number = -number;
    }
    /* handle zero, NaN, and +/- infinity */
    if (number == 0.0f) {
        UnityPrint("0");
    } else if (isnan(number)) {
        UnityPrint("nan");
    } else if (isinf(number)) {
        UnityPrint("inf");
    } else {
        UNITY_INT32 n_int = 0, n;
        int exponent = 0;
        int decimals, digits;
        char buf[16] = {0};

        /*
         * Scale up or down by powers of 10.  To minimize rounding error,
         * start with a factor/divisor of 10^10, which is the largest
         * power of 10 that can be represented exactly.  Finally, compute
         * (exactly) the remaining power of 10 and perform one more
         * multiplication or division.
         */
        if (number < 1.0f) {
            UNITY_DOUBLE factor = 1.0f;

            while (number < (UNITY_DOUBLE)max_scaled / 1e10f)  { number *= 1e10f; exponent -= 10; }
            while (number * factor < (UNITY_DOUBLE)min_scaled) { factor *= 10.0f; exponent--; }
            number *= factor;
        } else if (number > (UNITY_DOUBLE)max_scaled) {
            UNITY_DOUBLE divisor = 1.0f;

            while (number > (UNITY_DOUBLE)min_scaled * 1e10f)   { number  /= 1e10f; exponent += 10; }
            while (number / divisor > (UNITY_DOUBLE)max_scaled) { divisor *= 10.0f; exponent++; }

            number /= divisor;
        } else {
            /*
             * In this range, we can split off the integer part before
             * doing any multiplications.  This reduces rounding error by
             * freeing up significant bits in the fractional part.
             */
            UNITY_DOUBLE factor = 1.0f;
            n_int = (UNITY_INT32)number;
            number -= (UNITY_DOUBLE)n_int;

            while (n_int < min_scaled) { n_int *= 10; factor *= 10.0f; exponent--; }

            number *= factor;
        }

        /* round to nearest integer */
        n = ((UNITY_INT32)(number + number) + 1) / 2;

#ifndef UNITY_ROUND_TIES_AWAY_FROM_ZERO
        /* round to even if exactly between two integers */
        if ((n & 1) && (((UNITY_DOUBLE)n - number) == 0.5f))
            n--;
#endif

        n += n_int;

        if (n >= max_scaled)
        {
            n = min_scaled;
            exponent++;
        }

        /* determine where to place decimal point */
        decimals = ((exponent <= 0) && (exponent >= -(sig_digits + 3))) ? (-exponent) : (sig_digits - 1);
        exponent += decimals;

        /* truncate trailing zeroes after decimal point */
        while ((decimals > 0) && ((n % 10) == 0))
        {
            n /= 10;
            decimals--;
        }

        /* build up buffer in reverse order */
        digits = 0;
        while ((n != 0) || (digits < (decimals + 1)))
        {
            buf[digits++] = (char)('0' + n % 10);
            n /= 10;
        }
        while (digits > 0)
        {
            if (digits == decimals) { UNITY_OUTPUT_CHAR('.'); }
            UNITY_OUTPUT_CHAR(buf[--digits]);
        }

        /* print exponent if needed */
        if (exponent != 0)
        {
            UNITY_OUTPUT_CHAR('e');

            if (exponent < 0) {
                UNITY_OUTPUT_CHAR('-');
                exponent = -exponent;
            } else {
                UNITY_OUTPUT_CHAR('+');
            }

            digits = 0;
            while ((exponent != 0) || (digits < 2)) {
                buf[digits++] = (char)('0' + exponent % 10);
                exponent /= 10;
            }
            while (digits > 0) {
                UNITY_OUTPUT_CHAR(buf[--digits]);
            }
        }
    }
}
#endif /* ! UNITY_EXCLUDE_FLOAT_PRINT */

/*-----------------------------------------------*/
static void UnityTestResultsBegin(const char* file, const UNITY_LINE_TYPE line)
{
    UnityPrint(file);
    UNITY_OUTPUT_CHAR(':');
    UnityPrintNumber((UNITY_INT)line);
    UNITY_OUTPUT_CHAR(':');
    UnityPrint(Unity.CurrentTestName);
    UNITY_OUTPUT_CHAR(':');
}

/*-----------------------------------------------*/
#if 0
static void UnityTestResultsFailBegin(const UNITY_LINE_TYPE line)
{
    UnityTestResultsBegin(Unity.TestFile, line);
    UnityPrint(UnityStrFail);
    UNITY_OUTPUT_CHAR(':');
}
#endif

/*-----------------------------------------------*/
void UnityConcludeTest(void)
{
    if (Unity.CurrentTestIgnored)
    {
        Unity.TestIgnores++;
    }
    else if (!Unity.CurrentTestFailed)
    {
        UnityTestResultsBegin(Unity.TestFile, Unity.CurrentTestLineNumber);
        UnityPrint(UnityStrPass);
    }
    else
    {
        Unity.TestFailures++;
    }

    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;
    UNITY_EXEC_TIME_RESET();
    UNITY_PRINT_EOL();
    UNITY_FLUSH_CALL();
}


/*-----------------------------------------------
 * Assertion & Control Helpers
 *-----------------------------------------------*/

/*-----------------------------------------------*/
static int UnityIsOneArrayNull(UNITY_INTERNAL_PTR expected,
                               UNITY_INTERNAL_PTR actual,
                               const UNITY_LINE_TYPE lineNumber,
                               const char* msg)
{
    if (expected == actual) return 0; /* Both are NULL or same pointer */

    if (expected == NULL) {
        Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_NOT_EQUAL_TO};
        // UnityTestResultsFailBegin(lineNumber);
        // UnityPrint(UnityStrNullPointerForExpected);
        // UnityAddMsgIfSpecified(msg);
        return 1;
    }
    if (actual == NULL) {
        Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_NOT_EQUAL_TO};
        // UnityTestResultsFailBegin(lineNumber);
        // UnityPrint(UnityStrNullPointerForActual);
        // UnityAddMsgIfSpecified(msg);
        return 1;
    }
    return 0; /* return false if neither is NULL */
}

/*-----------------------------------------------
 * Assertion Functions
 *-----------------------------------------------*/

/*-----------------------------------------------*/
void UnityAssertBits(const UNITY_INT mask,
                     const UNITY_INT expected,
                     const UNITY_INT actual,
                     const char* msg,
                     const UNITY_LINE_TYPE lineNumber)
{
    RETURN_IF_FAIL_OR_IGNORE;
    if ((mask & expected) != (mask & actual)) {
        Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_EQUAL_TO};
        // UnityTestResultsFailBegin(lineNumber);
        // UnityPrint(UnityStrExpected);
        // UnityPrintMask((UNITY_UINT)mask, (UNITY_UINT)expected);
        // UnityPrint(UnityStrWas);
        // UnityPrintMask((UNITY_UINT)mask, (UNITY_UINT)actual);
        // UnityAddMsgIfSpecified(msg);
        UNITY_FAIL_AND_BAIL;
    }
}

/*-----------------------------------------------*/
void UnityAssertEqualNumber(const UNITY_INT expected,
                            const UNITY_INT actual,
                            const char* msg,
                            const UNITY_LINE_TYPE lineNumber,
                            const UNITY_DISPLAY_STYLE_T style)
{
    RETURN_IF_FAIL_OR_IGNORE;
    if (expected != actual) {
        Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_EQUAL_TO};
        //// UnityTestResultsFailBegin(lineNumber);
        //// UnityPrint(UnityStrExpected);
        //// UnityPrintNumberByStyle(expected, style);
        //// UnityPrint(UnityStrWas);
        //// UnityPrintNumberByStyle(actual, style);
        //// UnityAddMsgIfSpecified(msg);
        UNITY_FAIL_AND_BAIL;
    }
}

/*-----------------------------------------------*/
void UnityAssertGreaterOrLessOrEqualNumber(const UNITY_INT threshold,
                                           const UNITY_INT actual,
                                           const UNITY_COMPARISON_T compare,
                                           const char *msg,
                                           const UNITY_LINE_TYPE lineNumber,
                                           const UNITY_DISPLAY_STYLE_T style)
{
    int failed = 0;
    RETURN_IF_FAIL_OR_IGNORE;

    if ((threshold == actual) && (compare & UNITY_EQUAL_TO)) { return; }
    if ((threshold == actual))                               { failed = 1; }

    if ((style & UNITY_DISPLAY_RANGE_INT) == UNITY_DISPLAY_RANGE_INT)
    {
        if ((actual > threshold) && (compare & UNITY_SMALLER_THAN)) { failed = 1; }
        if ((actual < threshold) && (compare & UNITY_GREATER_THAN)) { failed = 1; }
    }
    else /* UINT or HEX */
    {
        if (((UNITY_UINT)actual > (UNITY_UINT)threshold) && (compare & UNITY_SMALLER_THAN)) { failed = 1; }
        if (((UNITY_UINT)actual < (UNITY_UINT)threshold) && (compare & UNITY_GREATER_THAN)) { failed = 1; }
    }

    if (failed) {
        Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_WITHIN};
#if 0
        UnityTestResultsFailBegin(lineNumber);
        UnityPrint(UnityStrExpected);
        UnityPrintNumberByStyle(actual, style);
        if (compare & UNITY_GREATER_THAN) { UnityPrint(UnityStrGt);      }
        if (compare & UNITY_SMALLER_THAN) { UnityPrint(UnityStrLt);      }
        if (compare & UNITY_EQUAL_TO)     { UnityPrint(UnityStrOrEqual); }
        UnityPrintNumberByStyle(threshold, style);
        UnityAddMsgIfSpecified(msg);
#endif
        UNITY_FAIL_AND_BAIL;
    }
}

#define UnityPrintPointlessAndBail()       \
{                                          \
    UnityTestResultsFailBegin(lineNumber); \
    UnityPrint(UnityStrPointless);         \
    UnityAddMsgIfSpecified(msg);           \
    UNITY_FAIL_AND_BAIL; }

/*-----------------------------------------------*/
void UnityAssertEqualIntArray(UNITY_INTERNAL_PTR expected,
                              UNITY_INTERNAL_PTR actual,
                              const UNITY_UINT32 num_elements,
                              const char* msg,
                              const UNITY_LINE_TYPE lineNumber,
                              const UNITY_DISPLAY_STYLE_T style,
                              const UNITY_FLAGS_T flags)
{
    UNITY_UINT32 elements = num_elements;
    unsigned int length   = style & 0xF;
    RETURN_IF_FAIL_OR_IGNORE;

    if (num_elements == 0) {
        Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_EQUAL_TO};
        // UnityPrintPointlessAndBail();
        UNITY_FAIL_AND_BAIL;
    }
    if (expected == actual) {
        return; /* Both are NULL or same pointer */
    }
    if (UnityIsOneArrayNull(expected, actual, lineNumber, msg)) {
        UNITY_FAIL_AND_BAIL;
    }

    while ((elements > 0) && (elements--))
    {
        UNITY_INT expect_val;
        UNITY_INT actual_val;
        switch (length)
        {
            case 1:
                expect_val = *(UNITY_PTR_ATTRIBUTE const UNITY_INT8*)expected;
                actual_val = *(UNITY_PTR_ATTRIBUTE const UNITY_INT8*)actual;
                break;
            case 2:
                expect_val = *(UNITY_PTR_ATTRIBUTE const UNITY_INT16*)expected;
                actual_val = *(UNITY_PTR_ATTRIBUTE const UNITY_INT16*)actual;
                break;
#ifdef UNITY_SUPPORT_64
            case 8:
                expect_val = *(UNITY_PTR_ATTRIBUTE const UNITY_INT64*)expected;
                actual_val = *(UNITY_PTR_ATTRIBUTE const UNITY_INT64*)actual;
                break;
#endif
            default: /* length 4 bytes */
                expect_val = *(UNITY_PTR_ATTRIBUTE const UNITY_INT32*)expected;
                actual_val = *(UNITY_PTR_ATTRIBUTE const UNITY_INT32*)actual;
                length = 4;
                break;
        }

        if (expect_val != actual_val) {
            if ((style & UNITY_DISPLAY_RANGE_UINT) && (length < sizeof(expect_val)))
            {   /* For UINT, remove sign extension (padding 1's) from signed type casts above */
                UNITY_INT mask = 1;
                mask = (mask << 8 * length) - 1;
                expect_val &= mask;
                actual_val &= mask;
            }
            Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_EQUAL_TO};
#if 0
            UnityTestResultsFailBegin(lineNumber);
            UnityPrint(UnityStrElement);
            UnityPrintNumberUnsigned(num_elements - elements - 1);
            UnityPrint(UnityStrExpected);
            UnityPrintNumberByStyle(expect_val, style);
            UnityPrint(UnityStrWas);
            UnityPrintNumberByStyle(actual_val, style);
            UnityAddMsgIfSpecified(msg);
#endif
            UNITY_FAIL_AND_BAIL;
        }
        if (flags == UNITY_ARRAY_TO_ARRAY)
        {
            expected = (UNITY_INTERNAL_PTR)(length + (const char*)expected);
        }
        actual   = (UNITY_INTERNAL_PTR)(length + (const char*)actual);
    }
}

/*-----------------------------------------------*/
#ifndef UNITY_EXCLUDE_FLOAT
/* Wrap this define in a function with variable types as float or double */
#define UNITY_FLOAT_OR_DOUBLE_WITHIN(delta, expected, actual, diff)                           \
    if (isinf(expected) && isinf(actual) && (((expected) < 0) == ((actual) < 0))) return 1;   \
    if (UNITY_NAN_CHECK) return 1;                                                            \
    (diff) = (actual) - (expected);                                                           \
    if ((diff) < 0) (diff) = -(diff);                                                         \
    if ((delta) < 0) (delta) = -(delta);                                                      \
    return !(isnan(diff) || isinf(diff) || ((diff) > (delta)))
    /* This first part of this condition will catch any NaN or Infinite values */
#ifndef UNITY_NAN_NOT_EQUAL_NAN
  #define UNITY_NAN_CHECK isnan(expected) && isnan(actual)
#else
  #define UNITY_NAN_CHECK 0
#endif

#ifndef UNITY_EXCLUDE_FLOAT_PRINT
  #define UNITY_PRINT_EXPECTED_AND_ACTUAL_FLOAT(expected, actual) \
  {                                                               \
    UnityPrint(UnityStrExpected);                                 \
    UnityPrintFloat(expected);                                    \
    UnityPrint(UnityStrWas);                                      \
    UnityPrintFloat(actual); }
#else
  #define UNITY_PRINT_EXPECTED_AND_ACTUAL_FLOAT(expected, actual) \
    UnityPrint(UnityStrDelta)
#endif /* UNITY_EXCLUDE_FLOAT_PRINT */

/*-----------------------------------------------*/
static int UnityFloatsWithin(UNITY_FLOAT delta, UNITY_FLOAT expected, UNITY_FLOAT actual)
{
    UNITY_FLOAT diff;
    UNITY_FLOAT_OR_DOUBLE_WITHIN(delta, expected, actual, diff);
}

/*-----------------------------------------------*/
void UnityAssertEqualFloatArray(UNITY_PTR_ATTRIBUTE const UNITY_FLOAT* expected,
                                UNITY_PTR_ATTRIBUTE const UNITY_FLOAT* actual,
                                const UNITY_UINT32 num_elements,
                                const char* msg,
                                const UNITY_LINE_TYPE lineNumber,
                                const UNITY_FLAGS_T flags)
{
    UNITY_UINT32 elements = num_elements;
    UNITY_PTR_ATTRIBUTE const UNITY_FLOAT* ptr_expected = expected;
    UNITY_PTR_ATTRIBUTE const UNITY_FLOAT* ptr_actual = actual;

    RETURN_IF_FAIL_OR_IGNORE;

    if (elements == 0) {
        Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_NOT_EQUAL_TO};
        // UnityPrintPointlessAndBail();
        UNITY_FAIL_AND_BAIL;
    }
    if (expected == actual) {
        return; /* Both are NULL or same pointer */
    }
    if (UnityIsOneArrayNull((UNITY_INTERNAL_PTR)expected, (UNITY_INTERNAL_PTR)actual, lineNumber, msg))
    {
        UNITY_FAIL_AND_BAIL;
    }

    while (elements--) {
        if (!UnityFloatsWithin(*ptr_expected * UNITY_FLOAT_PRECISION, *ptr_expected, *ptr_actual))
        {
            Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_WITHIN};
            // UnityTestResultsFailBegin(lineNumber);
            // UnityPrint(UnityStrElement);
            // UnityPrintNumberUnsigned(num_elements - elements - 1);
            // UNITY_PRINT_EXPECTED_AND_ACTUAL_FLOAT((UNITY_DOUBLE)*ptr_expected, (UNITY_DOUBLE)*ptr_actual);
            // UnityAddMsgIfSpecified(msg);
            UNITY_FAIL_AND_BAIL;
        }
        if (flags == UNITY_ARRAY_TO_ARRAY) {
            ptr_expected++;
        }
        ptr_actual++;
    }
}

/*-----------------------------------------------*/
void UnityAssertFloatsWithin(const UNITY_FLOAT delta,
                             const UNITY_FLOAT expected,
                             const UNITY_FLOAT actual,
                             const char* msg,
                             const UNITY_LINE_TYPE lineNumber)
{
    RETURN_IF_FAIL_OR_IGNORE;

    if (!UnityFloatsWithin(delta, expected, actual)) {
        Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_WITHIN};
        // UnityTestResultsFailBegin(lineNumber);
        // UNITY_PRINT_EXPECTED_AND_ACTUAL_FLOAT((UNITY_DOUBLE)expected, (UNITY_DOUBLE)actual);
        // UnityAddMsgIfSpecified(msg);
        UNITY_FAIL_AND_BAIL;
    }
}

/*-----------------------------------------------*/
void UnityAssertFloatSpecial(const UNITY_FLOAT actual,
                             const char* msg,
                             const UNITY_LINE_TYPE lineNumber,
                             const UNITY_FLOAT_TRAIT_T style)
{
    // const char* trait_names[] = {UnityStrInf, UnityStrNegInf, UnityStrNaN, UnityStrDet};
    UNITY_INT should_be_trait = ((UNITY_INT)style & 1);
    UNITY_INT is_trait        = !should_be_trait;
    // UNITY_INT trait_index     = (UNITY_INT)(style >> 1);

    RETURN_IF_FAIL_OR_IGNORE;

    switch (style) {
        case UNITY_FLOAT_IS_INF:
        case UNITY_FLOAT_IS_NOT_INF:
            is_trait = isinf(actual) && (actual > 0);
            break;
        case UNITY_FLOAT_IS_NEG_INF:
        case UNITY_FLOAT_IS_NOT_NEG_INF:
            is_trait = isinf(actual) && (actual < 0);
            break;

        case UNITY_FLOAT_IS_NAN:
        case UNITY_FLOAT_IS_NOT_NAN:
            is_trait = isnan(actual) ? 1 : 0;
            break;

        case UNITY_FLOAT_IS_DET: /* A determinate number is non infinite and not NaN. */
        case UNITY_FLOAT_IS_NOT_DET:
            is_trait = !isinf(actual) && !isnan(actual);
            break;

        default:
            // trait_index = 0;
            // trait_names[0] = UnityStrInvalidFloatTrait;
            break;
    }

    if (is_trait != should_be_trait) {
        Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_NOT_EQUAL_TO};
        // UnityTestResultsFailBegin(lineNumber);
        // UnityPrint(UnityStrExpected);
        // if (!should_be_trait)
        // {
        //     UnityPrint(UnityStrNot);
        // }
        // UnityPrint(trait_names[trait_index]);
        // UnityPrint(UnityStrWas);
#ifndef UNITY_EXCLUDE_FLOAT_PRINT
        // UnityPrintFloat((UNITY_DOUBLE)actual);
#else
        // if (should_be_trait)
        // {
        //     UnityPrint(UnityStrNot);
        // }
        // UnityPrint(trait_names[trait_index]);
#endif
        // UnityAddMsgIfSpecified(msg);
        UNITY_FAIL_AND_BAIL;
    }
}

#endif /* not UNITY_EXCLUDE_FLOAT */

/*-----------------------------------------------*/
#ifndef UNITY_EXCLUDE_DOUBLE
static int UnityDoublesWithin(UNITY_DOUBLE delta, UNITY_DOUBLE expected, UNITY_DOUBLE actual)
{
    UNITY_DOUBLE diff;
    UNITY_FLOAT_OR_DOUBLE_WITHIN(delta, expected, actual, diff);
}

/*-----------------------------------------------*/
void UnityAssertEqualDoubleArray(UNITY_PTR_ATTRIBUTE const UNITY_DOUBLE* expected,
                                 UNITY_PTR_ATTRIBUTE const UNITY_DOUBLE* actual,
                                 const UNITY_UINT32 num_elements,
                                 const char* msg,
                                 const UNITY_LINE_TYPE lineNumber,
                                 const UNITY_FLAGS_T flags)
{
    UNITY_UINT32 elements = num_elements;
    UNITY_PTR_ATTRIBUTE const UNITY_DOUBLE* ptr_expected = expected;
    UNITY_PTR_ATTRIBUTE const UNITY_DOUBLE* ptr_actual = actual;

    RETURN_IF_FAIL_OR_IGNORE;

    if (elements == 0) {
        Unity.CurrErr = {.msg=msg, .cmp=UNITY_NOT_EQUAL_TO};
        // UnityPrintPointlessAndBail();
        UNITY_FAIL_AND_BAIL;
    }
    if (expected == actual) {
        return; /* Both are NULL or same pointer */
    }
    if (UnityIsOneArrayNull((UNITY_INTERNAL_PTR)expected, (UNITY_INTERNAL_PTR)actual, lineNumber, msg))
    {
        UNITY_FAIL_AND_BAIL;
    }

    while (elements--) {
        if (!UnityDoublesWithin(*ptr_expected * UNITY_DOUBLE_PRECISION, *ptr_expected, *ptr_actual))
        {
            Unity.CurrErr = {.msg=msg, .cmp=UNITY_WITHIN};
            // UnityTestResultsFailBegin(lineNumber);
            // UnityPrint(UnityStrElement);
            // UnityPrintNumberUnsigned(num_elements - elements - 1);
            // UNITY_PRINT_EXPECTED_AND_ACTUAL_FLOAT(*ptr_expected, *ptr_actual);
            // UnityAddMsgIfSpecified(msg);
            UNITY_FAIL_AND_BAIL;
        }
        if (flags == UNITY_ARRAY_TO_ARRAY)
        {
            ptr_expected++;
        }
        ptr_actual++;
    }
}

/*-----------------------------------------------*/
void UnityAssertDoublesWithin(const UNITY_DOUBLE delta,
                              const UNITY_DOUBLE expected,
                              const UNITY_DOUBLE actual,
                              const char* msg,
                              const UNITY_LINE_TYPE lineNumber)
{
    RETURN_IF_FAIL_OR_IGNORE;

    if (!UnityDoublesWithin(delta, expected, actual)) {
        Unity.CurrErr = {.msg=msg, .cmp=UNITY_WITHIN};
        // UnityTestResultsFailBegin(lineNumber);
        // UNITY_PRINT_EXPECTED_AND_ACTUAL_FLOAT(expected, actual);
        // UnityAddMsgIfSpecified(msg);
        UNITY_FAIL_AND_BAIL;
    }
}

/*-----------------------------------------------*/
void UnityAssertDoubleSpecial(const UNITY_DOUBLE actual,
                              const char* msg,
                              const UNITY_LINE_TYPE lineNumber,
                              const UNITY_FLOAT_TRAIT_T style)
{
    const char* trait_names[] = {UnityStrInf, UnityStrNegInf, UnityStrNaN, UnityStrDet};
    UNITY_INT should_be_trait = ((UNITY_INT)style & 1);
    UNITY_INT is_trait        = !should_be_trait;
    UNITY_INT trait_index     = (UNITY_INT)(style >> 1);

    RETURN_IF_FAIL_OR_IGNORE;

    switch (style) {
        case UNITY_FLOAT_IS_INF:
        case UNITY_FLOAT_IS_NOT_INF:
            is_trait = isinf(actual) && (actual > 0);
            break;
        case UNITY_FLOAT_IS_NEG_INF:
        case UNITY_FLOAT_IS_NOT_NEG_INF:
            is_trait = isinf(actual) && (actual < 0);
            break;

        case UNITY_FLOAT_IS_NAN:
        case UNITY_FLOAT_IS_NOT_NAN:
            is_trait = isnan(actual) ? 1 : 0;
            break;

        case UNITY_FLOAT_IS_DET: /* A determinate number is non infinite and not NaN. */
        case UNITY_FLOAT_IS_NOT_DET:
            is_trait = !isinf(actual) && !isnan(actual);
            break;

        default:
            trait_index = 0;
            trait_names[0] = UnityStrInvalidFloatTrait;
            break;
    }

    if (is_trait != should_be_trait) {
        Unity.CurrErr = {.msg=msg, .cmp=UNITY_NOT_EQUAL_TO};
        // UnityTestResultsFailBegin(lineNumber);
        // UnityPrint(UnityStrExpected);
        // if (!should_be_trait) {
        //     UnityPrint(UnityStrNot);
        // }
        // UnityPrint(trait_names[trait_index]);
        // UnityPrint(UnityStrWas);
#ifndef UNITY_EXCLUDE_FLOAT_PRINT
        // UnityPrintFloat(actual);
#else
        // if (should_be_trait)
        // {
        //     UnityPrint(UnityStrNot);
        // }
        // UnityPrint(trait_names[trait_index]);
#endif
        // UnityAddMsgIfSpecified(msg);
        UNITY_FAIL_AND_BAIL;
    }
}

#endif /* not UNITY_EXCLUDE_DOUBLE */

/*-----------------------------------------------*/
void UnityAssertNumbersWithin(const UNITY_UINT delta,
                              const UNITY_INT expected,
                              const UNITY_INT actual,
                              const char* msg,
                              const UNITY_LINE_TYPE lineNumber,
                              const UNITY_DISPLAY_STYLE_T style)
{
    RETURN_IF_FAIL_OR_IGNORE;
    if ((style & UNITY_DISPLAY_RANGE_INT) == UNITY_DISPLAY_RANGE_INT) {
        if (actual > expected) {
            Unity.CurrentTestFailed = (((UNITY_UINT)actual - (UNITY_UINT)expected) > delta);
        } else {
            Unity.CurrentTestFailed = (((UNITY_UINT)expected - (UNITY_UINT)actual) > delta);
        }
    }
    else {
        if ((UNITY_UINT)actual > (UNITY_UINT)expected) {
            Unity.CurrentTestFailed = (((UNITY_UINT)actual - (UNITY_UINT)expected) > delta);
        } else {
            Unity.CurrentTestFailed = (((UNITY_UINT)expected - (UNITY_UINT)actual) > delta);
        }
    }
    if (Unity.CurrentTestFailed) {
        Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_WITHIN};
        // UnityTestResultsFailBegin(lineNumber);
        // UnityPrint(UnityStrDelta);
        // UnityPrintNumberByStyle((UNITY_INT)delta, style);
        // UnityPrint(UnityStrExpected);
        // UnityPrintNumberByStyle(expected, style);
        // UnityPrint(UnityStrWas);
        // UnityPrintNumberByStyle(actual, style);
        // UnityAddMsgIfSpecified(msg);
        UNITY_FAIL_AND_BAIL;
    }
}

/*-----------------------------------------------*/
void UnityAssertEqualString(const char* expected,
                            const char* actual,
                            const char* msg,
                            const UNITY_LINE_TYPE lineNumber)
{
    UNITY_UINT32 i;
    RETURN_IF_FAIL_OR_IGNORE;

    /* if both pointers not null compare the strings */
    if (expected && actual) {
        for (i = 0; expected[i] || actual[i]; i++) {
            if (expected[i] != actual[i]) {
                Unity.CurrentTestFailed = 1;
                break;
            }
        }
    }
    else
    { /* handle case of one pointers being null (if both null, test should pass) */
        if (expected != actual) {
            Unity.CurrentTestFailed = 1;
        }
    }
    if (Unity.CurrentTestFailed) {
        Unity.CurrErr = (struct UNITY_CURR_ERR_T){.msg=msg, .cmp=UNITY_EQUAL_TO};
        // UnityTestResultsFailBegin(lineNumber);
        // UnityPrintExpectedAndActualStrings(expected, actual);
        // UnityAddMsgIfSpecified(msg);
        UNITY_FAIL_AND_BAIL;
    }
}

/*-----------------------------------------------*/
void UnityAssertEqualStringLen(const char* expected,
                               const char* actual,
                               const UNITY_UINT32 length,
                               const char* msg,
                               const UNITY_LINE_TYPE lineNumber)
{
    UNITY_UINT32 i = 0;
    RETURN_IF_FAIL_OR_IGNORE;

    /* if both pointers not null compare the strings */
    if (expected && actual) {
        for (i = 0; (i < length) && (expected[i] || actual[i]); i++)
        {
            if (expected[i] != actual[i]) {
                Unity.CurrentTestFailed = 1;
                break;
            }
        }
    }
    else
    { /* handle case of one pointers being null (if both null, test should pass) */
        if (expected != actual) {
            Unity.CurrentTestFailed = 1;
        }
    }

    if (Unity.CurrentTestFailed) {
        Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_EQUAL_TO};
        // UnityTestResultsFailBegin(lineNumber);
        // UnityPrintExpectedAndActualStringsLen(expected, actual, length);
        // UnityAddMsgIfSpecified(msg);
        UNITY_FAIL_AND_BAIL;
    }
}

/*-----------------------------------------------*/
void UnityAssertEqualStringArray(UNITY_INTERNAL_PTR expected,
                                 const char** actual,
                                 const UNITY_UINT32 num_elements,
                                 const char* msg,
                                 const UNITY_LINE_TYPE lineNumber,
                                 const UNITY_FLAGS_T flags)
{
    UNITY_UINT32 i, j = 0;
    const char *expd = NULL, *act = NULL;

    RETURN_IF_FAIL_OR_IGNORE;

    /* if no elements, it's an error */
    if (num_elements == 0) {
        Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_NOT_EQUAL_TO};
        // UnityPrintPointlessAndBail();
        UNITY_FAIL_AND_BAIL;
    }
    if ((const void*)expected == (const void*)actual)
    {
        return; /* Both are NULL or same pointer */
    }
    if (UnityIsOneArrayNull((UNITY_INTERNAL_PTR)expected, (UNITY_INTERNAL_PTR)actual, lineNumber, msg))
    {
        UNITY_FAIL_AND_BAIL;
    }
    if (flags != UNITY_ARRAY_TO_ARRAY) {
        expd = (const char*)expected;
    }

    do
    {
        act = actual[j];
        if (flags == UNITY_ARRAY_TO_ARRAY)
        {
            expd = ((const char* const*)expected)[j];
        }

        /* if both pointers not null compare the strings */
        if (expd && act)
        {
            for (i = 0; expd[i] || act[i]; i++)
            {
                if (expd[i] != act[i])
                {
                    Unity.CurrentTestFailed = 1;
                    break;
                }
            }
        }
        else
        { /* handle case of one pointers being null (if both null, test should pass) */
            if (expd != act) {
                Unity.CurrentTestFailed = 1;
            }
        }

        if (Unity.CurrentTestFailed) {
            Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_NOT_EQUAL_TO};
            // UnityTestResultsFailBegin(lineNumber);
            // if (num_elements > 1)
            // {
            //     UnityPrint(UnityStrElement);
            //     UnityPrintNumberUnsigned(j);
            // }
            // UnityPrintExpectedAndActualStrings(expd, act);
            // UnityAddMsgIfSpecified(msg);
            UNITY_FAIL_AND_BAIL;
        }
    } while (++j < num_elements);
}

/*-----------------------------------------------*/
void UnityAssertEqualMemory(UNITY_INTERNAL_PTR expected,
                            UNITY_INTERNAL_PTR actual,
                            const UNITY_UINT32 length,
                            const UNITY_UINT32 num_elements,
                            const char* msg,
                            const UNITY_LINE_TYPE lineNumber,
                            const UNITY_FLAGS_T flags)
{
    UNITY_PTR_ATTRIBUTE const unsigned char* ptr_exp = (UNITY_PTR_ATTRIBUTE const unsigned char*)expected;
    UNITY_PTR_ATTRIBUTE const unsigned char* ptr_act = (UNITY_PTR_ATTRIBUTE const unsigned char*)actual;
    UNITY_UINT32 elements = num_elements;
    UNITY_UINT32 bytes;

    RETURN_IF_FAIL_OR_IGNORE;

    if ((elements == 0) || (length == 0)) {
        Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_NOT_EQUAL_TO};
        // UnityPrintPointlessAndBail();
        UNITY_FAIL_AND_BAIL;
    }
    if (expected == actual) {
        return; /* Both are NULL or same pointer */
    }
    if (UnityIsOneArrayNull(expected, actual, lineNumber, msg)) {
        UNITY_FAIL_AND_BAIL;
    }

    while (elements--) {
        bytes = length;
        while (bytes--) {
            if (*ptr_exp != *ptr_act) {
                // store the result to a global structure list instead of printing it out
                Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_EQUAL_TO};
                UNITY_FAIL_AND_BAIL;
            }
            ptr_exp++;
            ptr_act++;
        }
        if (flags == UNITY_ARRAY_TO_VAL)
        {
            ptr_exp = (UNITY_PTR_ATTRIBUTE const unsigned char*)expected;
        }
    }
}

/*-----------------------------------------------*/

static union
{
    UNITY_INT8 i8;
    UNITY_INT16 i16;
    UNITY_INT32 i32;
#ifdef UNITY_SUPPORT_64
    UNITY_INT64 i64;
#endif
#ifndef UNITY_EXCLUDE_FLOAT
    float f;
#endif
#ifndef UNITY_EXCLUDE_DOUBLE
    double d;
#endif
} UnityQuickCompare;

UNITY_INTERNAL_PTR UnityNumToPtr(const UNITY_INT num, const UNITY_UINT8 size)
{
    switch(size)
    {
        case 1:
          UnityQuickCompare.i8 = (UNITY_INT8)num;
          return (UNITY_INTERNAL_PTR)(&UnityQuickCompare.i8);

        case 2:
          UnityQuickCompare.i16 = (UNITY_INT16)num;
          return (UNITY_INTERNAL_PTR)(&UnityQuickCompare.i16);

#ifdef UNITY_SUPPORT_64
        case 8:
          UnityQuickCompare.i64 = (UNITY_INT64)num;
          return (UNITY_INTERNAL_PTR)(&UnityQuickCompare.i64);
#endif
        default: /* 4 bytes */
          UnityQuickCompare.i32 = (UNITY_INT32)num;
          return (UNITY_INTERNAL_PTR)(&UnityQuickCompare.i32);
    }
}

#ifndef UNITY_EXCLUDE_FLOAT
/*-----------------------------------------------*/
UNITY_INTERNAL_PTR UnityFloatToPtr(const float num)
{
    UnityQuickCompare.f = num;
    return (UNITY_INTERNAL_PTR)(&UnityQuickCompare.f);
}
#endif

#ifndef UNITY_EXCLUDE_DOUBLE
/*-----------------------------------------------*/
UNITY_INTERNAL_PTR UnityDoubleToPtr(const double num)
{
    UnityQuickCompare.d = num;
    return (UNITY_INTERNAL_PTR)(&UnityQuickCompare.d);
}
#endif

/*-----------------------------------------------
 * Control Functions
 *-----------------------------------------------*/

/*-----------------------------------------------*/
void UnityFail(const char* msg, const UNITY_LINE_TYPE line) {
    RETURN_IF_FAIL_OR_IGNORE;
    Unity.CurrErr = (struct UNITY_CURR_ERR_T) {.msg=msg, .cmp=UNITY_UNKNOWN};
    UNITY_FAIL_AND_BAIL;
}

/*-----------------------------------------------*/
void UnityIgnore(const char* msg, const UNITY_LINE_TYPE line)
{
    RETURN_IF_FAIL_OR_IGNORE;

    UnityTestResultsBegin(Unity.TestFile, line);
    UnityPrint(UnityStrIgnore);
    if (msg != NULL)
    {
        UNITY_OUTPUT_CHAR(':');
        UNITY_OUTPUT_CHAR(' ');
        UnityPrint(msg);
    }
    UNITY_IGNORE_AND_BAIL;
}

/*-----------------------------------------------*/
void UnityDefaultTestRun(UnityTestFunction Func, const char* FuncName, const int FuncLineNum)
{
    Unity.CurrentTestName = FuncName;
    Unity.CurrentTestLineNumber = (UNITY_LINE_TYPE)FuncLineNum;
    Unity.NumberOfTests++;
    UNITY_CLR_DETAILS();
    if (TEST_PROTECT())
    {
        setUp();
        Func();
    }
    if (TEST_PROTECT())
    {
        tearDown();
    }
    UnityConcludeTest();
}

/*-----------------------------------------------*/
void UnityBegin(const char* filename)
{
    Unity.TestFile = filename;
    Unity.CurrentTestName = NULL;
    Unity.CurrentTestLineNumber = 0;
    Unity.NumberOfTests = 0;
    Unity.TestFailures = 0;
    Unity.TestIgnores = 0;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;
    UNITY_EXEC_TIME_RESET();

    UNITY_CLR_DETAILS();
    UNITY_OUTPUT_START();
}

/*-----------------------------------------------*/
int UnityEnd(void)
{
    // we can directly check the summary data by debugger if we don't use print function
    return (int)(Unity.TestFailures);
}

/*-----------------------------------------------*/
