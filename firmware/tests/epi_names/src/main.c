/*
 * SPDX-License-Identifier: Apache-2.0
 * ZTEST — epi_names (folding UTF-8 -> ASCII e comparacao de nomes).
 */

#include "epi_names.h"

#include <string.h>

#include <zephyr/ztest.h>

ZTEST(epi_names, test_to_ascii_plain_passthrough)
{
	char out[EPI_NAME_LEN];

	epi_name_to_ascii("Capacete", out, sizeof(out));
	zassert_true(strcmp(out, "Capacete") == 0);
}

ZTEST(epi_names, test_to_ascii_lowercase_accents)
{
	char out[EPI_NAME_LEN];

	/* á à â ã -> a, ç -> c, é è -> e, í ì -> i, ó ò õ -> o, ú ù -> u */
	epi_name_to_ascii("\xC3\xA1\xC3\xA0\xC3\xA2\xC3\xA3", out, sizeof(out));
	zassert_true(strcmp(out, "aaaa") == 0);

	epi_name_to_ascii("\xC3\xA7", out, sizeof(out));
	zassert_true(strcmp(out, "c") == 0);

	epi_name_to_ascii("\xC3\xA9\xC3\xA8", out, sizeof(out));
	zassert_true(strcmp(out, "ee") == 0);

	epi_name_to_ascii("\xC3\xAD\xC3\xAC", out, sizeof(out));
	zassert_true(strcmp(out, "ii") == 0);

	epi_name_to_ascii("\xC3\xB3\xC3\xB2\xC3\xB5", out, sizeof(out));
	zassert_true(strcmp(out, "ooo") == 0);

	epi_name_to_ascii("\xC3\xBA\xC3\xB9", out, sizeof(out));
	zassert_true(strcmp(out, "uu") == 0);
}

ZTEST(epi_names, test_to_ascii_uppercase_accents)
{
	char out[EPI_NAME_LEN];

	/* Á Ç É Í Ó Ú -> A C E I O U */
	epi_name_to_ascii("\xC3\x81\xC3\x87\xC3\x89\xC3\x8D\xC3\x93\xC3\x9A", out,
			  sizeof(out));
	zassert_true(strcmp(out, "ACEIOU") == 0);
}

ZTEST(epi_names, test_to_ascii_oculos_word)
{
	char out[EPI_NAME_LEN];

	/* "Ó" "culos" em UTF-8 */
	epi_name_to_ascii("\xC3\x93" "culos", out, sizeof(out));
	zassert_true(strcmp(out, "Oculos") == 0);
}

ZTEST(epi_names, test_to_ascii_underscore_becomes_space)
{
	char out[EPI_NAME_LEN];

	epi_name_to_ascii("Protetor_auricular", out, sizeof(out));
	zassert_true(strcmp(out, "Protetor auricular") == 0);
}

ZTEST(epi_names, test_to_ascii_unknown_c3_byte)
{
	char out[EPI_NAME_LEN];

	/* C3 com continuacao fora da tabela (ñ = C3 B1) -> '?' */
	epi_name_to_ascii("\xC3\xB1", out, sizeof(out));
	zassert_true(strcmp(out, "?") == 0);
}

ZTEST(epi_names, test_to_ascii_non_c3_multibyte)
{
	char out[EPI_NAME_LEN];

	/* "€" (E2 82 AC): cada byte >= 0x80 fora de C3 vira '?' isolado */
	epi_name_to_ascii("\xE2\x82\xAC", out, sizeof(out));
	zassert_true(strcmp(out, "???") == 0);
}

ZTEST(epi_names, test_to_ascii_truncates_and_terminates)
{
	char out[4];

	epi_name_to_ascii("Capacete", out, sizeof(out));
	zassert_equal(3, strlen(out));
	zassert_true(strcmp(out, "Cap") == 0);
}

ZTEST(epi_names, test_to_ascii_null_safety)
{
	char out[4] = "abc";

	/* nao deve crashar nem alterar out */
	epi_name_to_ascii(NULL, out, sizeof(out));
	epi_name_to_ascii("x", NULL, 4);
	epi_name_to_ascii("x", out, 0);
}

ZTEST(epi_names, test_name_equal_exact)
{
	zassert_true(epi_name_equal("Capacete", "Capacete"));
	zassert_false(epi_name_equal("Capacete", "Bota"));
}

ZTEST(epi_names, test_name_equal_accent_insensitive)
{
	/* "Ó" "culos" (UTF-8) == "Oculos" (ASCII do shell) */
	zassert_true(epi_name_equal("\xC3\x93" "culos", "Oculos"));
	zassert_true(epi_name_equal("Oculos", "\xC3\x93" "culos"));
}

ZTEST(epi_names, test_name_equal_case_sensitive)
{
	/* folding preserva caixa: "capacete" != "Capacete" */
	zassert_false(epi_name_equal("capacete", "Capacete"));
}

ZTEST(epi_names, test_name_equal_null)
{
	zassert_false(epi_name_equal(NULL, "Capacete"));
	zassert_false(epi_name_equal("Capacete", NULL));
	zassert_false(epi_name_equal(NULL, NULL));
}

ZTEST(epi_names, test_item_in_list)
{
	char items[2][EPI_NAME_LEN];

	strncpy(items[0], "Capacete", EPI_NAME_LEN - 1);
	items[0][EPI_NAME_LEN - 1] = '\0';
	strncpy(items[1], "Oculos", EPI_NAME_LEN - 1);
	items[1][EPI_NAME_LEN - 1] = '\0';

	zassert_true(epi_item_in_list("Capacete", items, 2));
	zassert_true(epi_item_in_list("\xC3\x93" "culos", items, 2));
	zassert_false(epi_item_in_list("Bota", items, 2));
	zassert_false(epi_item_in_list("Capacete", items, 0));
}

ZTEST_SUITE(epi_names, NULL, NULL, NULL, NULL, NULL);
