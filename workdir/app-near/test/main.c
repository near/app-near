// clang-format off
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>
// clang-format on

#include <stdio.h>
#include "constants.h"
#include "context.h"
#include "parse_transaction.h"

// Temporary area to sore stuff and reuse the same memory
tmpContext_t tmp_ctx;
uiContext_t ui_context;

// Transactions to test
static const char TRANSFER_1_TRANSACTION_HEX[] =
    "18000000746573742d636f6e6e6563742d6c65646765722e746573740072068e029a9809c7"
    "f2da79c8a274743ff5f7edce24d648bd13e97f7093a0313204000000000000000200000076"
    "6750dc6e47ddccb2421f27dec206d5ea4694422a1c2fc35ca8c2e3638a02aacf4b01000000"
    "03000040bd8b5b936b6c00000000000000";
static const char TRANSFER_2_TRANSACTION_HEX[] =
    "17000000746573742d70722d3531372d6c65646765722e746573740072068e029a9809c7f2"
    "da79c8a274743ff5f7edce24d648bd13e97f7093a031320100000000000000020000007667"
    "4241eecd753a851429e4b415eb139dda037f705960200aa7a946bf24469b646e0100000003"
    "000000a1edccce1bc2d3000000000000";
static const char FUNCTION_CALL_TRANSACTION_HEX[] =
    "02000000766700dc9f5db36de8b6b403cf579db5425229175dacf5d1c7f10f1a685cbd7cca"
    "827e54000000000000000d00000072656365697665722e686572653a79b73517027a94f372"
    "03b95b6bb02fd4fb3764f1d689e04adcf54bf372573801000000020b0000006d6574686f64"
    "5f6e616d650f0000007b2261726773223a2268657265227d00008a5d784563010000004a48"
    "011416954508000000000000c185e79a1fa205b9845d992cfa848c0a7bb036a1e0b8a3a4c1"
    "9fcfb73d03ab5ab1559e8b6b3c37fa6c711159a191ab56be6565512bafc0178ac8326c73c4"
    "aa08";
static const char CREATE_ACC_TRANSACTION_HEX[] =
    "1000000072616e646f6d5f616363312e6e65617200fec7bcd20067927cfab115c72f77c997"
    "5b4157e665a0ba3aba382ec03986297901000000000000001000000072616e646f6d5f6163"
    "63322e6e6561720fa473fd26901df296be6adc4cc4df34d040efa2435224b6986910e630c2"
    "fef60100000000";
static const char DEPLOY_CONTRACT_TRANSACTION_HEX[] =
    "1000000072616e646f6d5f616363312e6e65617200fec7bcd20067927cfab115c72f77c997"
    "5b4157e665a0ba3aba382ec03986297901000000000000001000000072616e646f6d5f6163"
    "63322e6e6561720fa473fd26901df296be6adc4cc4df34d040efa2435224b6986910e630c2"
    "fef60100000001080000000000000000000000";
static const char STAKE_TRANSACTION_HEX[] =
    "1000000072616e646f6d5f616363312e6e65617200fec7bcd20067927cfab115c72f77c997"
    "5b4157e665a0ba3aba382ec03986297901000000000000001000000072616e646f6d5f6163"
    "63322e6e6561720fa473fd26901df296be6adc4cc4df34d040efa2435224b6986910e630c2"
    "fef601000000046400000000000000000000000000000000fec7bcd20067927cfab115c72f"
    "77c9975b4157e665a0ba3aba382ec039862979";
static const char ADD_LIMITED_KEY_TRANSACTION_HEX[] =
    "1000000072616e646f6d5f616363312e6e65617200fec7bcd20067927cfab115c72f77c997"
    "5b4157e665a0ba3aba382ec03986297901000000000000001000000072616e646f6d5f6163"
    "63322e6e6561720fa473fd26901df296be6adc4cc4df34d040efa2435224b6986910e630c2"
    "fef6010000000500fec7bcd20067927cfab115c72f77c9975b4157e665a0ba3aba382ec039"
    "86297900000000000000000001809698000000000000000000000000001700000072616e64"
    "6f6d5f72656369657665725f69642e6e65617202000000070000006d6574686f6431070000"
    "006d6574686f6432";
static const char ADD_UNLIMITED_KEY_TRANSACTION_HEX[] =
    "1000000072616e646f6d5f616363312e6e65617200fec7bcd20067927cfab115c72f77c997"
    "5b4157e665a0ba3aba382ec03986297901000000000000001000000072616e646f6d5f6163"
    "63322e6e6561720fa473fd26901df296be6adc4cc4df34d040efa2435224b6986910e630c2"
    "fef6010000000500fec7bcd20067927cfab115c72f77c9975b4157e665a0ba3aba382ec039"
    "862979000000000000000000001700000072616e646f6d5f72656369657665725f69642e6e"
    "65617202000000070000006d6574686f6431070000006d6574686f6432";
static const char DELETE_KEY_TRANSACTION_HEX[] =
    "1000000072616e646f6d5f616363312e6e65617200fec7bcd20067927cfab115c72f77c997"
    "5b4157e665a0ba3aba382ec03986297901000000000000001000000072616e646f6d5f6163"
    "63322e6e6561720fa473fd26901df296be6adc4cc4df34d040efa2435224b6986910e630c2"
    "fef6010000000600fec7bcd20067927cfab115c72f77c9975b4157e665a0ba3aba382ec039"
    "862979";
static const char DELETE_ACC_TRANSACTION_HEX[] =
    "1000000072616e646f6d5f616363312e6e65617200fec7bcd20067927cfab115c72f77c997"
    "5b4157e665a0ba3aba382ec03986297901000000000000001000000072616e646f6d5f6163"
    "63322e6e6561720fa473fd26901df296be6adc4cc4df34d040efa2435224b6986910e630c2"
    "fef60100000007110000006f746865724163636f756e742e6e656172";
static const char MULTIPLE_ACCTIONS_TRANSACTION_HEX[] =
    "02000000766700dc9f5db36de8b6b403cf579db5425229175dacf5d1c7f10f1a685cbd7cca"
    "827e55000000000000000d00000072656365697665722e68657265e2f0bc3a1a2e946a6e7b"
    "33e645d3aeb7146f1331d1f9e4c8fc7c1970f063925602000000020b0000006d6574686f64"
    "5f6e616d650f0000007b2261726773223a2268657265227d00008a5d784563010000004a48"
    "0114169545080000000000030100000000000000000000000000000000f9a87dae39c9f8bc"
    "f0f34ebca57b047e2921947ab605ac6b9468083b1ef81f6884c5c0df20c5c7e8faf1204743"
    "a8aa7bf348711afb5c04f171991f44fd6ddf03";

int tests_run = 0;

static int parse_hex(char *output_buffer, size_t output_length,
                     const char *hex_str) {
  for (int i = 0; i < output_length; i++) {
    if (hex_str[i * 2] == 0) {
      return i;
    }
    if (hex_str[i * 2 + 1] == 0) {
      return -1;
    }
    sscanf(&hex_str[i * 2], "%2hhx", &output_buffer[i]);
  }
  return -1;
}

#define PARSE_HEX(output_array, hex_str)                                  \
  parse_hex(output_array, sizeof(output_array) / sizeof(output_array[0]), \
            hex_str)

static void print_ui() {
  printf("---\n");
  printf("%s\n", ui_context.line1);
  printf("%s\n", ui_context.line2);
  printf("%s\n", ui_context.line3);
  printf("%s\n", ui_context.long_line);
  printf("%s\n", ui_context.line5);
  printf("%s\n", ui_context.amount);
}

static void test_parse_transfer_1(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      PARSE_HEX(tmp_ctx.signing_context.buffer, TRANSFER_1_TRANSACTION_HEX);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "transfer");  // action
  assert_string_equal(ui_context.line2, "vg");        // receiver
  assert_string_equal(ui_context.line3, "test-connect-ledger.test");  // signer
  assert_string_equal(ui_context.amount, "0.002");                    // amount
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_TRANSFER);
}

static void test_parse_transfer_2(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      PARSE_HEX(tmp_ctx.signing_context.buffer, TRANSFER_2_TRANSACTION_HEX);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "transfer");                 // action
  assert_string_equal(ui_context.line2, "vg");                       // receiver
  assert_string_equal(ui_context.line3, "test-pr-517-ledger.test");  // signer
  assert_string_equal(ui_context.amount, "1");                       // amount
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_TRANSFER);
}

static void test_parse_function_call(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      PARSE_HEX(tmp_ctx.signing_context.buffer, FUNCTION_CALL_TRANSACTION_HEX);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "method_name");    // action
  assert_string_equal(ui_context.line2, "receiver.here");  // receiver
  assert_string_equal(ui_context.line3, "vg");             // signer
  assert_string_equal(ui_context.amount, "");              // amount
  assert_string_equal(ui_context.long_line,
                      "{\"args\":\"here\"}");   // JSON args
  assert_string_equal(ui_context.line5, "10");  // deposit
  assert_int_equal(active_flow, SIGN_FLOW_FUNCTION_CALL);
}

static void test_parse_create_account(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      PARSE_HEX(tmp_ctx.signing_context.buffer, CREATE_ACC_TRANSACTION_HEX);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "create account");    // action
  assert_string_equal(ui_context.line2, "random_acc2.near");  // new account id
  assert_string_equal(ui_context.line3, "random_acc1.near");  // master account
  assert_string_equal(ui_context.amount, "");                 // amount
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_GENERIC);
}

static void test_parse_deploy_contract(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used = PARSE_HEX(
      tmp_ctx.signing_context.buffer, DEPLOY_CONTRACT_TRANSACTION_HEX);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "deploy contract");   // action
  assert_string_equal(ui_context.line2, "random_acc2.near");  // receiver
  assert_string_equal(ui_context.line3, "random_acc1.near");  // signer
  assert_string_equal(ui_context.amount, "");
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_GENERIC);
}

static void test_parse_stake(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      PARSE_HEX(tmp_ctx.signing_context.buffer, STAKE_TRANSACTION_HEX);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "stake");             // action
  assert_string_equal(ui_context.line2, "random_acc2.near");  // receiver
  assert_string_equal(ui_context.line3, "random_acc1.near");  // signer
  assert_string_equal(ui_context.amount, "");
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_GENERIC);
}

static void test_parse_add_limited_key(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used = PARSE_HEX(
      tmp_ctx.signing_context.buffer, ADD_LIMITED_KEY_TRANSACTION_HEX);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "add key");                  // action
  assert_string_equal(ui_context.line2, "random_reciever_id.near");  // receiver
  assert_string_equal(ui_context.line3, "random_acc1.near");         // signer
  assert_string_equal(ui_context.amount, "");
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "0.00000000000000001");  // limitation
  assert_int_equal(active_flow, SIGN_FLOW_ADD_FUNCTION_CALL_KEY);
}

static void test_parse_add_unlimited_key(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used = PARSE_HEX(
      tmp_ctx.signing_context.buffer, ADD_UNLIMITED_KEY_TRANSACTION_HEX);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "add key");                  // action
  assert_string_equal(ui_context.line2, "random_reciever_id.near");  // receiver
  assert_string_equal(ui_context.line3, "random_acc1.near");         // signer
  assert_string_equal(ui_context.amount, "");
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "Unlimited");  // unlimited
  assert_int_equal(active_flow, SIGN_FLOW_ADD_FUNCTION_CALL_KEY);
}

static void test_parse_delete_key(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      PARSE_HEX(tmp_ctx.signing_context.buffer, DELETE_KEY_TRANSACTION_HEX);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "delete key");        // action
  assert_string_equal(ui_context.line2, "random_acc2.near");  // receiver
  assert_string_equal(ui_context.line3, "random_acc1.near");  // signer
  assert_string_equal(ui_context.amount, "");
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_GENERIC);
}

static void test_parse_delete_account(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      PARSE_HEX(tmp_ctx.signing_context.buffer, DELETE_ACC_TRANSACTION_HEX);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "delete account");    // action
  assert_string_equal(ui_context.line2, "random_acc2.near");  // receiver
  assert_string_equal(ui_context.line3, "random_acc1.near");  // signer
  assert_string_equal(ui_context.amount, "");
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_GENERIC);
}

static void test_parse_multiple_actions(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used = PARSE_HEX(
      tmp_ctx.signing_context.buffer, MULTIPLE_ACCTIONS_TRANSACTION_HEX);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "multiple actions");
  assert_string_equal(ui_context.line2, "receiver.here");  // receiver
  assert_string_equal(ui_context.line3, "vg");             // signer
  // TODO: Show total amount?
  assert_string_equal(ui_context.amount, "");
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_GENERIC);
}

int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_parse_transfer_1),
      cmocka_unit_test(test_parse_transfer_2),
      cmocka_unit_test(test_parse_function_call),
      cmocka_unit_test(test_parse_create_account),
      cmocka_unit_test(test_parse_deploy_contract),
      cmocka_unit_test(test_parse_stake),
      cmocka_unit_test(test_parse_add_limited_key),
      cmocka_unit_test(test_parse_add_unlimited_key),
      cmocka_unit_test(test_parse_delete_key),
      cmocka_unit_test(test_parse_delete_account),
      cmocka_unit_test(test_parse_multiple_actions),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
