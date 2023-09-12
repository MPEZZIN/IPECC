/*
 *  Copyright (C) 2023 - This file is part of IPECC project
 *
 *  Authors:
 *      Karim KHALFALLAH <karim.khalfallah@ssi.gouv.fr>
 *      Ryad BENADJILA <ryadbenadjila@gmail.com>
 *
 *  Contributors:
 *      Adrian THILLARD
 *      Emmanuel PROUFF
 *
 *  This software is licensed under GPL v2 license.
 *  See LICENSE file at the root folder of the project.
 */

#include "../hw_accelerator_driver.h"
#include "../hw_accelerator_driver_ipecc_platform.h"
#include "ecc-test-linux.h"
#if 0
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <error.h>
#endif


/* Helper for curve set */
extern int ip_set_curve(curve_t*);
/* Point operations helpers */
/*   [k]P */
extern int ip_set_pt_and_run_kp(ipecc_test_t*);
extern int check_kp_result(ipecc_test_t*, stats_t*, bool*);
/*   P + Q */
extern int ip_set_pts_and_run_ptadd(ipecc_test_t*);
extern int check_ptadd_result(ipecc_test_t*, stats_t*, bool*);
/*   [2]P */
extern int ip_set_pt_and_run_ptdbl(ipecc_test_t*);
extern int check_ptdbl_result(ipecc_test_t*, stats_t*, bool*);
/*   (-P) */
extern int ip_set_pt_and_run_ptneg(ipecc_test_t*);
extern int check_ptneg_result(ipecc_test_t*, stats_t*, bool*);
/* Point tests helpers */
/*   is P on curve? */
extern int ip_set_pt_and_check_on_curve(ipecc_test_t*);
extern int check_test_oncurve(ipecc_test_t*, stats_t*, bool* res);
/*   are P & Q equal? */
extern int ip_set_pts_and_test_equal(ipecc_test_t*);
extern int check_test_equal(ipecc_test_t*, stats_t*, bool* res);
/*   are P & Q opposite? */
extern int ip_set_pts_and_test_oppos(ipecc_test_t*);
extern int check_test_oppos(ipecc_test_t*, stats_t*, bool* res);

/*
 * Pointer 'line' will be allocated by getline, but freed by us
 * as recommanded in the man GETLINE(3) page (see function
 * print_stats_and_exit() hereafter).
 */
char* line = NULL;

#define max(a,b) do { \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; }) \
 while (0)

static bool line_is_empty(char *l)
{
	uint32_t c;
	bool ret = true;
	for (c=0; ;c++) {
		if ((l[c] == '\r') || (l[c] == '\n')) {
			break;
		} else if ((l[c] != ' ') && (l[c] != '\t')) {
			ret = false;
			break;
		}
	}
	return ret;
}

static void print_stats_regularly(stats_t* st)
{
	if ((st->total % DISPLAY_MODULO) == 0) {
		printf("%s%8d%s %s%8d%s %8d\n\r",
				KGRN, st->ok, KNRM, KRED, st->nok, KNRM, st->total);
	}
}

void print_stats_and_exit(ipecc_test_t* t, stats_t* s, const char* msg, unsigned int linenum)
{
	printf("Stopped on test %d.%d%s\n\r", t->curve->id, t->id, KNRM);
	printf("OK = %d\n\r", s->ok);
	printf("nOK = %d\n\r", s->nok);
	printf("total = %d\n\r", s->total);
	printf("--\n\r");
	if (line) {
		free(line);
	}
	error_at_line(-1, EXIT_FAILURE, __FILE__, linenum, "%s", msg);
}

/*
 * Convert one hexadecimal digit into an integer.
 */
static int hex2dec(const char c, unsigned char *nb)
{
	if ( (c >= 'a') && (c <= 'f') ) {
		*nb = c - 'a' + 10;
	} else if ( (c >= 'A') && (c <= 'F') ) {
		*nb = c - 'A' + 10;
	} else if ( (c >= '0') && (c <= '9') ) {
		*nb = c - '0';
	} else {
		printf("%sError: '%c' not an hexadecimal digit%s\n\r", KERR, c, KNRM);
		goto err;
	}
	return 0;
err:
	return -1;
}

/*
 * Extract an hexadecimal string (without the 0x) from a position in a line 
 * (pointed to by parameter 'pc') convert it in binary form and fill buffer
 * 'nb_x' with it, parsing exactly 'nbchar' - 2 characters.
 *
 * Also set the size (in bytes) of the output buffer.
 */
static int hex_to_large_num(const char *pc, unsigned char* nb_x, unsigned int valnn, const ssize_t nbchar)
{
	int i, j;
	unsigned int k;
	uint8_t tmp;

#if 0
	/* Clear content of nb_x. */
	for (j=0; j<NBMAXSZ; j++) {
		nb_x[j] = 0;
	}
#endif
	/* Format bytes of large number; */
	j = 0;
	for (i = nbchar - 2 ; i>=0 ; i--) {
	//for (i = 0; i < nbchar - 1 ; i++) {
		if (hex2dec(pc[i], &tmp)) {
			printf("%sError while trying to convert character string '%s'"
					" into an hexadecimal number%s\n\r", KERR, pc, KNRM);
			goto err;
		} else {
#if 0
			if ((j % 2) == 0) {
				nb_x[j / 2] = 0;
			}
#endif
			nb_x[DIV(valnn, 8) - 1 - j/2] = ( (j % 2) ? nb_x[DIV(valnn, 8) - 1 - j/2] : 0) + ( tmp * (0x1U << (4*(j % 2))) );
			j++;
		}
	}
	 /* Fill possible remaining buffer space with 0s.
	 */
	for (k = 1 + (j-1)/2; k < DIV(valnn, 8); k++) {
		nb_x[k] = 0;
	}
	for (k=0; k<DIV(valnn, 8); k++) {
		PRINTF(" %02x", nb_x[k]);
	}
	PRINTF("\n\r");
#if 0
	/* Set the size of the number */
	*nb_x_sz = (unsigned int)(((j % 2) == 0) ? (j/2) : (j/2) + 1);
#endif

#if 0
	for (i=0 ; i<(*nb_x_sz) ; i++) {
		PRINTF("%s%02x%s", KWHT, nb_x[i], KNRM);
	}
	PRINTF("\n\r");
#endif

	return 0;
err:
	return -1;
}

/* Same as strtol() but as mentioned in the man STRTOL(3) page,
 * 'errno' is set to 0 before the call, and then checked after to
 * catch if an error occurred (see below 'print_stats_and_exit()'
 * function).
 */
static int strtol_with_err(const char *nptr, unsigned int* nb)
{
	errno = 0;
	*nb = strtol(nptr, NULL, 10);
	if (errno) {
		return -1;
	} else {
		return 0;
	}
}

int cmp_two_pts_coords(point_t* p0, point_t* p1, bool* res)
{
	uint32_t i;

	/*
	 * If four coordinates sizes do not match, that's an error
	 * (don't even compare).
	 */
	if ( (p0->x.sz != p0->y.sz) || (p0->x.sz != p1->x.sz) || (p0->y.sz != p1->x.sz)
			|| (p0->y.sz != p1->y.sz) || (p1->x.sz != p1->y.sz) )
	{
		printf("%sError: can't compare coord. buffers that are not of the same byte size to begin with.%s\n\r",
				KERR, KNRM);
		goto err;
	}
	/* Compare the X & Y coordinates one byte after the other. */
	*res = true;
	for (i = 0; i < p0->x.sz; i++) {
		if ((p0->x.val[i] != p1->x.val[i]) || (p0->y.val[i] != p1->y.val[i])) {
			*res = false;
			break;
		}
	}
	return 0;
err:
	return -1;
}


/* Curve definition */
static curve_t curve = INIT_CURVE();
/* Main test structure */
static ipecc_test_t test = {
	.curve = &curve,
	.ptp = INIT_POINT(),
	.ptq = INIT_POINT(),
	.k = INIT_LARGE_NUMBER(),
	.pt_sw_res = INIT_POINT(),
	.pt_hw_res = INIT_POINT(),
	.blinding = 0,
	.sw_answer = INIT_PTTEST(),
	.hw_answer = INIT_PTTEST(),
	.op = OP_NONE,
	.is_an_exception = false,
	.id = 0
};

/* Statistics */
static stats_t stats = {
	.ok = 0, .nok = 0, .total = 0
};

int main(int argc, char *argv[])
{
	uint32_t i;
	line_t line_type_expected;
	size_t len = 0;
	ssize_t nread;
	uint32_t debug_not_prod;
	uint32_t version_major, version_minor;

	(void)argc;
	(void)argv;

	bool result_pts_are_equal;
	bool result_tests_are_identical;

	printf("%sWelcome to the IPECC test applet.\n\r", KWHT);

	/* Move the claptrap below rather in --help. */
#if 0
	printf("Reads test-vectors from standard-input, has them computed by hardware,\n\r");
	printf("then checks that result matches what was expected.\n\r");
	printf("Text format for tests is described in the IPECC doc.\n\r");
	printf("(c.f Appendix \"Simulating & testing the IP\").%s\n\r", KNRM);
#endif

#if 1
	/* Is it a 'debug' or a 'production' version of the IP? */
	if (hw_driver_is_debug(&debug_not_prod)) {
		printf("%sError: Probing 'debug or production mode' triggered an error.%s\n\r", KERR, KNRM);
		exit(EXIT_FAILURE);
	}

	if (debug_not_prod){
		if (hw_driver_get_version_major(&version_major)){
			printf("%sError: Probing major number triggered an error.%s\n\r", KERR, KNRM);
			exit(EXIT_FAILURE);
		}
		if (hw_driver_get_version_minor(&version_minor)){
			printf("%sError: Probing minor number triggered an error.%s\n\r", KERR, KNRM);
			exit(EXIT_FAILURE);
		}
		log_print("Debug mode (HW version %d.%d)\n\r", version_major, version_minor);
		/*
		 * We must activate, in the TRNG, the pulling of raw random bytes by the
		 * post-processing function (because in debug mode it is disabled upon
		 * reset).
		 */
		if (hw_driver_trng_post_proc_enable()){
			printf("%sError: Enabling TRNG post-processing on hardware triggered an error.%s\n\r", KERR, KNRM);
			exit(EXIT_FAILURE);
		}
	} else {
		printf("Production mode.\n\r");
	}
#endif

	/* Main infinite loop, parsing lines from standard input to extract:
	 *   - input vectors
	 *   - type of operation
	 *   - expected result,
	 * then have the same computation done by hardware, and then check
	 * the result of hardware against the expected one.
	 */
	line_type_expected = EXPECT_NONE;
	while (((nread = getline(&line, &len, stdin))) != -1) {
		/*
		 * Allow comment lines starting with #
		 * (simply assert exception flag if it starts with "# EXCEPTION"
		 * because in this case this comment is meaningful).
		 */
		if (line[0] == '#') {
			if ( (strncmp(line, "# EXCEPTION", strlen("# EXCEPTION"))) == 0 ) {
				test.is_an_exception = true;
			}
			continue;
		}
		/*
		 * Allow empty lines
		 */
		if (line_is_empty(line) == true) {
			continue;
		}
		/*
		 * Process line according to some kind of finite state
		 * machine on input vector test format.
		 */
		switch (line_type_expected) {

			case EXPECT_NONE:{
				/*
				 * Parse line.
				 */
				if ( (strncmp(line, "== NEW CURVE #", strlen("== NEW CURVE #"))) == 0 ) {
					/*
					 * Extract the curve nb, after '#' character.
					 */
					strtol_with_err(line + strlen("== NEW CURVE #"), &curve.id); /* NEW CURVE #x */
					line_type_expected = EXPECT_NN;
					curve.valid = false;
				} else if ( (strncmp(line, "== TEST [k]P #", strlen("== TEST [k]P #"))) == 0 ) {
					/*
					 * Extract the computation nb, after '#' character.
					 */
					/* Determine position of the dot in the line. */
					for (i=0; ; i++) {
						if (*(line + strlen("== TEST [k]P #") + i) == '.') {
							*(line + strlen("== TEST [k]P #") + i) = '\0';
							break;
						}
					}
					strtol_with_err(line + strlen("== TEST [k]P #") + i + 1, &test.id);
					test.op = OP_KP;
					test.ptp.valid = false;
					test.k.valid = false;
					test.pt_sw_res.valid = false;
					test.pt_hw_res.valid = false;
					line_type_expected = EXPECT_PX;
					/*
					 * Blinding will be applied only if input file/stream test says so
					 * (otherwise default is no blinding).
					 */
					test.blinding = 0;
				} else if ( (strncmp(line, "== TEST P+Q #", strlen("== TEST P+Q #"))) == 0 ) {
					/*
					 * Extract the computation nb, after '#' character.
					 */
					/* Determine position of the dot in the line. */
					for (i=0; ; i++) {
						if (*(line + strlen("== TEST P+Q #") + i) == '.') {
							*(line + strlen("== TEST P+Q #") + i) = '\0';
							break;
						}
					}
					strtol_with_err(line + strlen("== TEST P+Q #") + i + 1, &test.id);
					test.op = OP_PTADD;
					test.ptp.valid = false;
					test.ptq.valid = false;
					test.pt_sw_res.valid = false;
					test.pt_hw_res.valid = false;
					line_type_expected = EXPECT_PX;
				} else if ( (strncmp(line, "== TEST [2]P #", strlen("== TEST [2]P #"))) == 0 ) {
					/*
					 * Extract the computation nb, after '#' character.
					 */
					/* Determine position of the dot in the line. */
					for (i=0; ; i++) {
						if (*(line + strlen("== TEST [2]P #") + i) == '.') {
							*(line + strlen("== TEST [2]P #") + i) = '\0';
							break;
						}
					}
					strtol_with_err(line + strlen("== TEST [2]P #") + i + 1, &test.id);
					test.op = OP_PTDBL;
					test.ptp.valid = false;
					test.pt_sw_res.valid = false;
					test.pt_hw_res.valid = false;
					line_type_expected = EXPECT_PX;
				} else if ( (strncmp(line, "== TEST -P #", strlen("== TEST -P #"))) == 0 ) {
					/*
					 * Extract the computation nb, after '#' character.
					 */
					/* Determine position of the dot in the line. */
					for (i=0; ; i++) {
						if (*(line + strlen("== TEST -P #") + i) == '.') {
							*(line + strlen("== TEST -P #") + i) = '\0';
							break;
						}
					}
					strtol_with_err(line + strlen("== TEST -P #") + i + 1, &test.id);
					test.op = OP_PTNEG;
					test.ptp.valid = false;
					test.pt_sw_res.valid = false;
					test.pt_hw_res.valid = false;
					line_type_expected = EXPECT_PX;
				} else if ( (strncmp(line, "== TEST isPoncurve #", strlen("== TEST isPoncurve #"))) == 0 ) {
					/*
					 * Extract the computation nb, after '#' character.
					 */
					/* Determine position of the dot in the line. */
					for (i=0; ; i++) {
						if (*(line + strlen("== TEST isPoncurve #") + i) == '.') {
							*(line + strlen("== TEST isPoncurve #") + i) = '\0';
							break;
						}
					}
					strtol_with_err(line + strlen("== TEST isPoncurve #") + i + 1, &test.id);
					test.op = OP_TST_CHK;
					test.ptp.valid = false;
					test.sw_answer.valid = false;
					test.hw_answer.valid = false;
					line_type_expected = EXPECT_PX;
				} else if ( (strncmp(line, "== TEST isP==Q #", strlen("== TEST isP==Q #"))) == 0 ) {
					/*
					 * Extract the computation nb, after '#' character.
					 */
					/* Determine position of the dot in the line. */
					for (i=0; ; i++) {
						if (*(line + strlen("== TEST isP==Q #") + i) == '.') {
							*(line + strlen("== TEST isP==Q #") + i) = '\0';
							break;
						}
					}
					strtol_with_err(line + strlen("== TEST isP==Q #") + i + 1, &test.id);
					test.op = OP_TST_EQU;
					test.ptp.valid = false;
					test.ptq.valid = false;
					test.sw_answer.valid = false;
					test.hw_answer.valid = false;
					line_type_expected = EXPECT_PX;
				} else if ( (strncmp(line, "== TEST isP==-Q #", strlen("== TEST isP==-Q #"))) == 0 ) {
					/*
					 * Extract the computation nb, after '#' character.
					 */
					/* Determine position of the dot in the line. */
					for (i=0; ; i++) {
						if (*(line + strlen("== TEST isP==-Q #") + i) == '.') {
							*(line + strlen("== TEST isP==-Q #") + i) = '\0';
							break;
						}
					}
					strtol_with_err(line + strlen("== TEST isP==-Q #") + i + 1, &test.id);
					test.op = OP_TST_OPP;
					test.ptp.valid = false;
					test.ptq.valid = false;
					test.sw_answer.valid = false;
					test.hw_answer.valid = false;
					line_type_expected = EXPECT_PX;
				} else {
					printf("%sError: Could not find any of the expected commands from "
							"input file/stream.\n\r", KERR);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_NONE')", __LINE__);
				}
				break;
			}

			case EXPECT_NN:{
				/*
				 * Parse line to extract value of nn
				 */
				if ( (strncmp(line, "nn=", strlen("nn="))) == 0 )
				{
					strtol_with_err(&line[3], &curve.nn);
					PRINTF("%snn=%d\n\r%s", KINF, curve.nn, KNRM);
					line_type_expected = EXPECT_P;
				} else {
					printf("%sError: Could not find the expected token \"nn=\" "
							"from input file/stream.\n\r", KERR);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_NN)'", __LINE__);
				}
				break;
			}

			case EXPECT_P:{
				/* Parse line to extract value of p */
				if ( (strncmp(line, "p=0x", strlen("p=0x"))) == 0 ) {
					PRINTF("%sp=0x%s%s", KINF, line + strlen("p=0x"), KNRM);
					/*
					 * Process the hexadecimal value of p to create the list
					 * of bytes to transfer to the IP.
					 */
					if (hex_to_large_num(
							line + strlen("p=0x"), test.curve->p.val, test.curve->nn, nread - strlen("p=0x")))
					{
						printf("%sError: Value of main curve parameter 'p' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_P')", __LINE__);
					}
					test.curve->p.sz = DIV(test.curve->nn, 8);
					test.curve->p.valid = true;
					line_type_expected = EXPECT_A;
				} else {
					printf("%sError: Could not find the expected token \"p=0x\" "
							"from input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_P')", __LINE__);
				}
				break;
			}

			case EXPECT_A:{
				/* Parse line to extract value of a */
				if ( (strncmp(line, "a=0x", strlen("a=0x"))) == 0 ) {
					PRINTF("%sa=0x%s%s", KINF, line + strlen("a=0x"), KNRM);
					/*
					 * Process the hexadecimal value of a to create the list
					 * of bytes to transfer to the IP.
					 */
					if (hex_to_large_num(
							line + strlen("a=0x"), test.curve->a.val, test.curve->nn, nread - strlen("a=0x")))
					{
						printf("%sError: Value of curve parameter 'a' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_A')", __LINE__);
					}
					test.curve->a.sz = DIV(test.curve->nn, 8);
					test.curve->a.valid = true;
					line_type_expected = EXPECT_B;
				} else {
					printf("%sError: Could not find the expected token \"a=0x\" "
							"from input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_A')", __LINE__);
				}
				break;
			}

			case EXPECT_B:{
				/* Parse line to extract value of b/ */
				if ( (strncmp(line, "b=0x", strlen("b=0x"))) == 0 ) {
					PRINTF("%sb=0x%s%s", KINF, line + strlen("b=0x"), KNRM);
					/*
					 * Process the hexadecimal value of b to create the list
					 * of bytes to transfer to the IP.
					 */
					if (hex_to_large_num(
							line + strlen("b=0x"), test.curve->b.val, test.curve->nn, nread - strlen("b=0x")))
					{
						printf("%sError: Value of curve parameter 'b' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_B')", __LINE__);
					}
					test.curve->b.sz = DIV(test.curve->nn, 8);
					test.curve->b.valid = true;
					line_type_expected = EXPECT_Q;
				} else {
					printf("%sError: Could not find the expected token \"b=0x\" "
							"from input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_B')", __LINE__);
				}
				break;
			}

			case EXPECT_Q:{
				/* Parse line to extract value of q. */
				if ( (strncmp(line, "q=0x", strlen("q=0x"))) == 0 )
				{
					PRINTF("%sq=0x%s%s", KINF, line + strlen("q=0x"), KNRM);
					/*
					 * Process the hexadecimal value of q to create the list
					 * of bytes to transfer to the IP (also set the size of
					 * the number).
					 */
					if (hex_to_large_num(
							line + strlen("q=0x"), test.curve->q.val, test.curve->nn, nread - strlen("q=0x")))
					{
						printf("%sError: Value of curve parameter 'q' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_Q')", __LINE__);
					}
					test.curve->q.sz = DIV(test.curve->nn, 8);
					test.curve->q.valid = true;
					test.curve->valid = true;
					/*
					 * Transfer curve parameters to the IP.
					 */
					if (ip_set_curve(test.curve))
					{
						printf("%sError: Could not transmit curve parameters to driver.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_Q')", __LINE__);
					}
					line_type_expected = EXPECT_NONE;
				} else {
					printf("%sError: Could not find the expected token \"q=0x\" "
							"from input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_B')", __LINE__);
				}
				break;
			}

			case EXPECT_PX:{
				/* Parse line to extract value of Px */
				if ( (strncmp(line, "Px=0x", strlen("Px=0x"))) == 0 ) {
					PRINTF("%sPx=0x%s%s", KINF, line + strlen("Px=0x"), KNRM);
					/*
					 * Process the hexadecimal value of Px to create the list
					 * of bytes to transfer to the IP.
					 */
					if (hex_to_large_num(
							line + strlen("Px=0x"), test.ptp.x.val, test.curve->nn, nread - strlen("Px=0x")))
					{
						printf("%sError: Value of point coordinate 'Px' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_PX')", __LINE__);
					}
					/*
					 * Position point P not to be null
					 */
					test.ptp.x.sz = DIV(test.curve->nn, 8);
					test.ptp.is_null = false;
					line_type_expected = EXPECT_PY;
				} else if ( (strncmp(line, "P=0", strlen("P=0"))) == 0 ) {
					PRINTF("%sP=0\n\r%s", KINF, KNRM);
					/*
					 * Position point P to be null
					 */
					test.ptp.is_null = true;
					test.ptp.valid = true;
					if (test.op == OP_KP) {
						line_type_expected = EXPECT_K;
					} else if (test.op == OP_PTADD) {
						line_type_expected = EXPECT_QX;
					} else if (test.op == OP_PTDBL) {
						line_type_expected = EXPECT_TWOP_X;
					} else if (test.op == OP_PTNEG) {
						line_type_expected = EXPECT_NEGP_X;
					} else if (test.op == OP_TST_CHK) {
						line_type_expected = EXPECT_TRUE_OR_FALSE;
					} else if ((test.op == OP_TST_EQU) || (test.op == OP_TST_OPP)) {
						line_type_expected = EXPECT_QX;
					} else {
						printf("%sError: unknown or undefined type of operation.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_PX')", __LINE__);
					}
				} else {
					printf("%sError: Could not find one of the expected tokens \"Px=0x\" "
							"or \"P=0\" from input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_PX')", __LINE__);
				}
				break;
			}

			case EXPECT_PY:{
				/* Parse line to extract value of Py */
				if ( (strncmp(line, "Py=0x", strlen("Py=0x"))) == 0 ) {
					PRINTF("%sPy=0x%s%s", KINF, line + strlen("Py=0x"), KNRM);
					/*
					 * Process the hexadecimal value of Py to create the list
					 * of bytes to transfer to the IP.
					 */
					if (hex_to_large_num(
							line + strlen("Py=0x"), test.ptp.y.val, test.curve->nn, nread - strlen("Py=0x")))
					{
						printf("%sError: Value of point coordinate 'Py' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_PY')", __LINE__);
					}
					test.ptp.y.sz = DIV(test.curve->nn, 8);
					test.ptp.valid = true;
					if (test.op == OP_KP) {
						line_type_expected = EXPECT_K;
					} else if (test.op == OP_PTADD) {
						line_type_expected = EXPECT_QX;
					} else if (test.op == OP_PTDBL) {
						line_type_expected = EXPECT_TWOP_X;
					} else if (test.op == OP_PTNEG) {
						line_type_expected = EXPECT_NEGP_X;
					} else if (test.op == OP_TST_CHK) {
						line_type_expected = EXPECT_TRUE_OR_FALSE;
					} else if ((test.op == OP_TST_EQU) || (test.op == OP_TST_OPP)) {
						line_type_expected = EXPECT_QX;
					} else {
						printf("%sError: unknown or undefined type of operation.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_PY')", __LINE__);
					}
				} else {
					printf("%sError: Could not find the expected token \"Py=0x\" "
								"from input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_PY')", __LINE__);
				}
				break;
			}

			case EXPECT_QX:{
				/* Parse line to extract value of Qx. */
				if ( (strncmp(line, "Qx=0x", strlen("Qx=0x"))) == 0 ) {
					PRINTF("%sQx=0x%s%s", KINF, line + strlen("Qx=0x"), KNRM);
					/*
					 * Process the hexadecimal value of Qx to create the list
					 * of bytes to transfer to the IP.
					 */
					if (hex_to_large_num(
							line + strlen("Qx=0x"), test.ptq.x.val, test.curve->nn, nread - strlen("Qx=0x")))
					{
						printf("%sError: Value of point coordinate 'Qx' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_QX')", __LINE__);
					}
					/*
					 * Position point Q not to be null.
					 */
					test.ptq.x.sz = DIV(test.curve->nn, 8);
					test.ptq.is_null = false;
					line_type_expected = EXPECT_QY;
				} else if ( (strncmp(line, "Q=0", strlen("Q=0"))) == 0 ) {
					PRINTF("%sQ=0\n\r%s", KINF, KNRM);
					/*
					 * Position point Q to be null.
					 */
					test.ptq.is_null = true;
					test.ptq.valid = true;
					if (test.op == OP_PTADD) {
						line_type_expected = EXPECT_P_PLUS_QX;
					} else if (test.op == OP_TST_EQU) {
						line_type_expected = EXPECT_TRUE_OR_FALSE;
					} else if (test.op == OP_TST_OPP) {
						line_type_expected = EXPECT_TRUE_OR_FALSE;
					} else {
						printf("%sError: unknown or undefined type of operation.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_QX')", __LINE__);
					}
				} else {
					printf("%sError: Could not find one of the expected tokens \"Qx=0x\" "
							"or \"Q=0\" from input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_QX')", __LINE__);
				}
				break;
			}

			case EXPECT_QY:{
				/*
				 * Parse line to extract value of Py.
				 */
				if ( (strncmp(line, "Qy=0x", strlen("Qy=0x"))) == 0 ) {
					PRINTF("%sQy=0x%s%s", KINF, line + strlen("Qy=0x"), KNRM);
					/*
					 * Process the hexadecimal value of Py to create the list
					 * of bytes to transfer to the IP.
					 */
					if (hex_to_large_num(
							line + strlen("Qy=0x"), test.ptq.y.val, test.curve->nn, nread - strlen("Qy=0x")))
					{
						printf("%sError: Value of point coordinate 'Qy' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_QY')", __LINE__);
					}
					test.ptq.y.sz = DIV(test.curve->nn, 8);
					test.ptq.valid = true;
					if (test.op == OP_PTADD) {
						line_type_expected = EXPECT_P_PLUS_QX;
					} else if (test.op == OP_TST_EQU) {
						line_type_expected = EXPECT_TRUE_OR_FALSE;
					} else if (test.op == OP_TST_OPP) {
						line_type_expected = EXPECT_TRUE_OR_FALSE;
					} else {
						printf("%sError: unknown or undefined type of operation.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_QY')", __LINE__);
					}
				} else {
					printf("%sError: Could not find the expected token \"Qy=0x\" "
							"from input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_QY')", __LINE__);
				}
				break;
			}

			case EXPECT_K:{
				/*
				 * Parse line to extract value of k.
				 */
				if ( (strncmp(line, "k=0x", strlen("k=0x"))) == 0 ) {
					PRINTF("%sk=0x%s%s", KINF, line + strlen("k=0x"), KNRM);
					/*
					 * Process the hexadecimal value of k to create the list
					 * of bytes to transfer to the IP.
					 */
					if (hex_to_large_num(
							line + strlen("k=0x"), test.k.val, test.curve->nn, nread - strlen("k=0x")))
					{
						printf("%sError: Value of scalar number 'k' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_K')", __LINE__);
					}
					test.k.sz = DIV(test.curve->nn, 8);
					test.k.valid = true;
					line_type_expected = EXPECT_KPX_OR_BLD;
				} else {
					printf("%sError: Could not find the expected token \"k=0x\" "
							"from input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_K')", __LINE__);
				}
				break;
			}

			case EXPECT_KPX_OR_BLD:{
				/*
				 * Parse line to extract possible nb of blinding bits.
				 * */
				if ( (strncmp(line, "nbbld=", strlen("nbbld="))) == 0 ) {
					PRINTF("%snbbld=%s%s", KINF, line + strlen("nbbld="), KNRM);
					if (strtol_with_err(line + strlen("nbbld="), &test.blinding))
					{
						printf("%sError: while converting \"nbbld=\" argument to a number.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_KPX_OR_BLD')", __LINE__);
					}
					/* Keep line_type_expected to EXPECT_KPX_OR_BLD to parse point [k]P coordinates */
				} else if ( (strncmp(line, "kPx=0x", strlen("kPx=0x"))) == 0 ) {
					PRINTF("%skPx=0x%s%s", KINF, line + strlen("kPx=0x"), KNRM);
					/*
					 * Process the hexadecimal value of kPx for comparison with HW.
					 */
					if (hex_to_large_num(
							line + strlen("kPx=0x"), test.pt_sw_res.x.val, test.curve->nn, nread - strlen("kPx=0x")))
					{
						printf("%sError: Value of point coordinate 'kPx' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_KPX_OR_BLD')", __LINE__);
					}
					/*
					 * Record that expected result point [k]P should not be null.
					 */
					test.pt_sw_res.x.sz = DIV(test.curve->nn, 8);
					test.pt_sw_res.is_null = false;
					line_type_expected = EXPECT_KPY;
				} else if ( (strncmp(line, "kP=0", strlen("kP=0"))) == 0 ) {
					PRINTF("%sExpected result point [k]P = 0\n\r%s", KINF, KNRM);
					/*
					 * Record that expected result point [k]P should be null.
					 */
					test.pt_sw_res.is_null = true;
					test.pt_sw_res.valid = true;
					/*
					 * Set and execute a [k]P computation test on hardware.
					 */
					if (ip_set_pt_and_run_kp(&test))
					{
						printf("%sError: Computation of scalar multiplication on hardware triggered an error.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_KPX_OR_BLD')", __LINE__);
					}
					/*
					 * Check IP result against the expected one (which is the point at infinity)
					 */
					if (check_kp_result(&test, &stats, &result_pts_are_equal))
					{
						printf("%sError: Couldn't compare [k]P hardware result w/ the expected one.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_KPX_OR_BLD')", __LINE__);
					}
					/*
					 * Stats
					 */
					stats.total++;
					line_type_expected = EXPECT_NONE;
					print_stats_regularly(&stats);
#if 0
					/*
					 * Mark the next test to come as not being an exception (a priori)
					 * so that [k]P timing statistics only consider [k]P computations
					 * with no exception.
					 */
					test.is_an_exception = false;
#endif
				} else {
					printf("%sError: Could not find one of the expected tokens \"nbbld=\" "
							"or \"kPx=0x\" or \"kP=0\" in input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_KPX_OR_BLD')", __LINE__);
				}
				break;
			}

			case EXPECT_KPY:{
				/* Parse line to extract value of [k]Py (y of result) */
				if ( (strncmp(line, "kPy=0x", strlen("kPy=0x"))) == 0 ) {
					PRINTF("%skPy=0x%s%s", KINF, line + strlen("kPy=0x"), KNRM);
					/*
					 * Process the hexadecimal value of kPy for comparison with HW
					 */
					if (hex_to_large_num(
							line + strlen("kPy=0x"), test.pt_sw_res.y.val, test.curve->nn, nread - strlen("kPy=0x")))
					{
						printf("%sError: Value of point coordinate 'kPy' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_KPY')", __LINE__);
					}
					test.pt_sw_res.y.sz = DIV(test.curve->nn, 8);
					test.pt_sw_res.valid = true;
					/*
					 * Set and execute a [k]P computation test on harware.
					 */
					if (ip_set_pt_and_run_kp(&test))
					{
						printf("%sError: Computation of scalar multiplication on hardware triggered an error.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_KPY')", __LINE__);
					}
					/*
					 * Check IP result against the expected one.
					 */
					if (check_kp_result(&test, &stats, &result_pts_are_equal))
					{
						printf("%sError: Couldn't compare [k]P hardware result w/ the expected one.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_KPY')", __LINE__);
					}
					/*
					 * Stats
					 */
					stats.total++;
					line_type_expected = EXPECT_NONE;
					print_stats_regularly(&stats);
#if 0
					/*
					 * Mark the next test to come as not being an exception (a priori)
					 * so that [k]P duration statistics only consider [k]P computations
					 * with no exception
					 */
					test.is_an_exception = false;
#endif
				} else {
					printf("%sError: Could not find the expected token \"kPy=0x\" "
							"in input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_KPY')", __LINE__);
				}
				break;
			}

			case EXPECT_P_PLUS_QX:{
				/*
				 * Parse line to extract value of (P+Q).x
				 */
				if ( (strncmp(line, "PplusQx=0x", strlen("PplusQx=0x"))) == 0 ) {
					PRINTF("%s(P+Q)x=0x%s%s", KINF, line + strlen("PplusQx=0x"), KNRM);
					/*
					 * Process the hexadecimal value of (P+Q).x for comparison with HW
					 */
					if (hex_to_large_num(
							line + strlen("PplusQx=0x"), test.pt_sw_res.x.val, test.curve->nn, nread - strlen("PplusQx=0x")))
					{
						printf("%sError: Value of point coordinate '(P+Q).x' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_P_PLUS_QX')", __LINE__);
					}
					test.pt_sw_res.x.sz = DIV(test.curve->nn, 8);
					test.pt_sw_res.is_null = false;
					line_type_expected = EXPECT_P_PLUS_QY;
				} else if ( (strncmp(line, "PplusQ=0", strlen("PplusQ=0"))) == 0 ) {
					PRINTF("%s(P+Q)=0%s", KINF, KNRM);
					test.pt_sw_res.is_null = true;
					test.pt_sw_res.valid = true;
					/*
					 * Set and execute a P + Q computation test on hardware.
					 */
					if (ip_set_pts_and_run_ptadd(&test))
					{
						printf("%sError: Computation of P + Q on hardware triggered an error.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_P_PLUS_QX')", __LINE__);
					}
					/*
					 * Check IP result against the expected one.
					 */
					if (check_ptadd_result(&test, &stats, &result_pts_are_equal))
					{
						printf("%sError: Couldn't compare P + Q hardware result w/ the expected one.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_P_PLUS_QX')", __LINE__);
					}
					/*
					 * Stats
					 */
					stats.total++;
					line_type_expected = EXPECT_NONE;
					print_stats_regularly(&stats);
#if 0
					/*
					 * Mark the next test to come as not being an exception (a priori)
					 * so that [k]P duration statistics only consider [k]P computations
					 * with no exception
					 */
					test.is_an_exception = false;
#endif
				} else {
					printf("%sError: Could not find one of the expected tokens \"PplusQx=0x\" "
							"or \"(P+Q)=0\" in input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_KPY')", __LINE__);
				}
				break;
			}

			case EXPECT_P_PLUS_QY:{
				/*
				 * Parse line to extract value of (P+Q).y
				 */
				if ( (strncmp(line, "PplusQy=0x", strlen("PplusQy=0x"))) == 0 ) {
					PRINTF("%s(P+Q)y=0x%s%s", KINF, line + strlen("PplusQy=0x"), KNRM);
					/*
					 * Process the hexadecimal value of (P+Q).y for comparison with HW
					 */
					if (hex_to_large_num(
							line + strlen("PplusQy=0x"), test.pt_sw_res.y.val, test.curve->nn,
							nread - strlen("PplusQy=0x")))
					{
						printf("%sError: Value of point coordinate '(P+Q).y' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_P_PLUS_QY')", __LINE__);
					}
					test.pt_sw_res.y.sz = DIV(test.curve->nn, 8);
					test.pt_sw_res.valid = true;
					/*
					 * Set and execute a P + Q computation test on harware.
					 */
					if (ip_set_pts_and_run_ptadd(&test))
					{
						printf("%sError: Computation of P + Q on hardware triggered an error.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_P_PLUS_QY')", __LINE__);
					}
					/*
					 * Check IP result against the expected one.
					 */
					if (check_ptadd_result(&test, &stats, &result_pts_are_equal))
					{
						printf("%sError: Couldn't compare P + Q hardware result w/ the expected one.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_P_PLUS_QY')", __LINE__);
					}
					/*
					 * Stats
					 */
					stats.total++;
					line_type_expected = EXPECT_NONE;
					print_stats_regularly(&stats);
#if 0
					/*
					 * Mark the next test to come as not being an exception (a priori)
					 * so that [k]P duration statistics only consider [k]P computations
					 * with no exception
					 */
					test.is_an_exception = false;
#endif
				} else {
					printf("%sError: Could not find the expected token \"PplusQy=0x\" "
							"in input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_P_PLUS_QY')", __LINE__);
				}
				break;
			}

			case EXPECT_TWOP_X:{
				/*
				 * Parse line to extract value of [2]P.x
				 */
				if ( (strncmp(line, "twoPx=0x", strlen("twoPx=0x"))) == 0 ) {
					PRINTF("%s[2]P.x=0x%s%s", KINF, line + strlen("twoPx=0x"), KNRM);
					/*
					 * Process the hexadecimal value of [2]P.x for comparison with HW
					 */
					if (hex_to_large_num(
							line + strlen("twoPx=0x"), test.pt_sw_res.x.val, test.curve->nn,
							nread - strlen("twoPx=0x")))
					{
						printf("%sError: Value of point coordinate '[2]P.x' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TWOP_X')", __LINE__);
					}
					test.pt_sw_res.x.sz = DIV(test.curve->nn, 8);
					test.pt_sw_res.is_null = false;
					line_type_expected = EXPECT_TWOP_Y;
				} else if ( (strncmp(line, "twoP=0", strlen("twoP=0"))) == 0 ) {
					PRINTF("%s[2]P=0\n\r%s", KINF, KNRM);
					test.pt_sw_res.is_null = true;
					test.pt_sw_res.valid = true;
					/*
					 * Set and execute a [2]P computation test on hardware.
					 */
					if (ip_set_pt_and_run_ptdbl(&test))
					{
						printf("%sError: Computation of [2]P on hardware triggered an error.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TWOP_X')", __LINE__);
					}
					/*
					 * Check IP result against the expected one.
					 */
					if (check_ptdbl_result(&test, &stats, &result_pts_are_equal))
					{
						printf("%sError: Couldn't compare [2]P hardware result w/ the expected one.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TWOP_X')", __LINE__);
					}
					/*
					 * Stats
					 */
					stats.total++;
					line_type_expected = EXPECT_NONE;
					print_stats_regularly(&stats);
#if 0
					/*
					 * Mark the next test to come as not being an exception (a priori)
					 * so that [k]P duration statistics only consider [k]P computations
					 * with no exception
					 */
					test.is_an_exception = false;
#endif
				} else {
					printf("%sError: Could not find one of the expected tokens \"twoPx=0x\" "
							"or \"twoP=0\" from input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TWOP_X')", __LINE__);
				}
				break;
			}

			case EXPECT_TWOP_Y:{
				/*
				 * Parse line to extract value of [2]P.y
				 */
				if ( (strncmp(line, "twoPy=0x", strlen("twoPy=0x"))) == 0 ) {
					PRINTF("%s[2]P.y=0x%s%s", KINF, line + strlen("twoPy=0x"), KNRM);
					/*
					 * Process the hexadecimal value of [2]P.y for comparison with HW
					 */
					if (hex_to_large_num(
							line + strlen("twoPy=0x"), test.pt_sw_res.y.val, test.curve->nn,
							nread - strlen("twoPy=0x")))
					{
						printf("%sError: Value of point coordinate '[2]P.y' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TWOP_Y')", __LINE__);
					}
					test.pt_sw_res.y.sz = DIV(test.curve->nn, 8);
					test.pt_sw_res.valid = true;
					/*
					 * Set and execute a [2]P computation test on hardware.
					 */
					if (ip_set_pt_and_run_ptdbl(&test))
					{
						printf("%sError: Computation of [2]P on hardware triggered an error.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TWOP_Y')", __LINE__);
					}
					/*
					 * Check IP result against the expected one.
					 */
					if (check_ptdbl_result(&test, &stats, &result_pts_are_equal))
					{
						printf("%sError: Couldn't compare [2]P hardware result w/ the expected one.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TWOP_Y')", __LINE__);
					}
					/*
					 * Stats
					 */
					stats.total++;
					line_type_expected = EXPECT_NONE;
					print_stats_regularly(&stats);
#if 0
					/*
					 * Mark the next test to come as not being an exception (a priori)
					 * so that [k]P duration statistics only consider [k]P computations
					 * with no exception
					 */
					test.is_an_exception = false;
#endif
				} else {
					printf("%sError: Could not find the expected token \"twoPy=0x\" "
							"in input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TWOP_Y')", __LINE__);
				}
				break;
			}

			case EXPECT_NEGP_X:{
				/*
				 * Parse line to extract value of -P.x
				 */
				if ( (strncmp(line, "negPx=0x", strlen("negPx=0x"))) == 0 ) {
					PRINTF("%s-P.x=0x%s%s", KINF, line + strlen("negPx=0x"), KNRM);
					/*
					 * Process the hexadecimal value of -P.x for comparison with HW
					 */
					if (hex_to_large_num(
							line + strlen("negPx=0x"), test.pt_sw_res.x.val, test.curve->nn,
							nread - strlen("negPx=0x")))
					{
						printf("%sError: Value of point coordinate '(-P).x' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_NEGP_X')", __LINE__);
					}
					test.pt_sw_res.x.sz = DIV(test.curve->nn, 8);
					test.pt_sw_res.is_null = false;
					line_type_expected = EXPECT_NEGP_Y;
				} else if ( (strncmp(line, "negP=0", strlen("negP=0"))) == 0 ) {
					PRINTF("%s-P=0\n\r%s", KINF, KNRM);
					test.pt_sw_res.is_null = true;
					test.pt_sw_res.valid = true;
					/*
					 * Set and execute a -P computation test on hardware.
					 */
					if (ip_set_pt_and_run_ptneg(&test))
					{
						printf("%sError: Computation of -P on hardware triggered an error.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_NEGP_X')", __LINE__);
					}
					/*
					 * Check IP result against the expected one.
					 */
					if (check_ptneg_result(&test, &stats, &result_pts_are_equal))
					{
						printf("%sError: Couldn't compare -P hardware result w/ the expected one.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_NEGP_X')", __LINE__);
					}
					/*
					 * Stats
					 */
					stats.total++;
					line_type_expected = EXPECT_NONE;
					print_stats_regularly(&stats);
#if 0
					/*
					 * Mark the next test to come as not being an exception (a priori)
					 * so that [k]P duration statistics only consider [k]P computations
					 * with no exception
					 */
					test.is_an_exception = false;
#endif
				} else {
					printf("%sError: Could not find one of the expected tokens \"negPx=0x\" "
							"or \"negP=0\" in input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_NEGP_X')", __LINE__);
				}
				break;
			}

			case EXPECT_NEGP_Y:{
				/*
				 * Parse line to extract value of -P.y
				 */
				if ( (strncmp(line, "negPy=0x", strlen("negPy=0x"))) == 0 ) {
					PRINTF("%s-P.y=0x%s%s", KINF, line + strlen("negPy=0x"), KNRM);
					/*
					 * Process the hexadecimal value of -P.y for comparison with HW
					 */
					if (hex_to_large_num(
							line + strlen("negPy=0x"), test.pt_sw_res.y.val, test.curve->nn,
							nread - strlen("negPy=0x")))
					{
						printf("%sError: Value of point coordinate '(-P).y' could not be extracted "
								"from input file/stream.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_NEGP_Y')", __LINE__);
					}
					test.pt_sw_res.y.sz = DIV(test.curve->nn, 8);
					test.pt_sw_res.valid = true;
					/*
					 * Set and execute a -P computation test on hardware.
					 */
					if (ip_set_pt_and_run_ptneg(&test))
					{
						printf("%sError: Computation of -P on hardware triggered an error.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_NEGP_Y')", __LINE__);
					}
					/*
					 * Check IP result against the expected one.
					 */
					if (check_ptneg_result(&test, &stats, &result_pts_are_equal))
					{
						printf("%sError: Couldn't compare -P hardware result w/ the expected one.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_NEGP_Y')", __LINE__);
					}
					/*
					 * Stats
					 */
					stats.total++;
					line_type_expected = EXPECT_NONE;
					print_stats_regularly(&stats);
#if 0
					/*
					 * Mark the next test to come as not being an exception (a priori)
					 * so that [k]P duration statistics only consider [k]P computations
					 * with no exception
					 */
					test.is_an_exception = false;
#endif
				} else {
					printf("%sError: Could not find the expected token \"negPy=0x\" "
							"from input file/stream.%s\n\r", KERR, KNRM);
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_NEGP_Y')", __LINE__);
				}
				break;
			}

			case EXPECT_TRUE_OR_FALSE:{
				/*
				 * Parse line to extract test answer (true or false)
				 */
				if ( (strncasecmp(line, "true", strlen("true"))) == 0 ) {
					PRINTF("%sanswer is true\n\r%s", KINF, KNRM);
					switch (test.op) {
						case OP_TST_CHK:
						case OP_TST_EQU:
						case OP_TST_OPP:
							test.sw_answer.answer = true;
							test.sw_answer.valid = true;
							break;
						default:{
							printf("%sError: Invalid test type.%s\n\r", KERR, KNRM);
							print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TRUE_OR_FALSE')", __LINE__);
							break;
						}
					}
				} else if ( (strncasecmp(line, "false", strlen("false"))) == 0 ) {
					PRINTF("%sanswer is false\n\r%s", KINF, KNRM);
					switch (test.op) {
						case OP_TST_CHK:
						case OP_TST_EQU:
						case OP_TST_OPP:
							test.sw_answer.answer = false;
							test.sw_answer.valid = true;
							break;
						default:
							printf("%sError: Invalid test type.%s\n\r", KERR, KNRM);
							print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TRUE_OR_FALSE')", __LINE__);
							break;
					}
				} else {
					printf("%sError: Could not find one of the expected tokens \"true\" "
							"or \"false\" from input file/stream for test \"%s\".%s\n\r", KERR, KNRM,
							( test.op == OP_TST_CHK ? "OP_TST_CHK" : (test.op == OP_TST_EQU ? "OP_TST_EQU" :
							  (test.op == OP_TST_OPP ? "OP_TST_OPP" : "UNKNOWN_TEST"))));
					print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TRUE_OR_FALSE')", __LINE__);
				}
				/*
				 * Set and execute one or two points on which to perform the test on hardware.
				 */
				switch (test.op) {
					case OP_TST_CHK:{
						if (ip_set_pt_and_check_on_curve(&test))
						{
							printf("%sError: Point test \"is on curve?\" on hardware triggered an error.%s\n\r", KERR, KNRM);
							print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TRUE_OR_FALSE')", __LINE__);
						}
						break;
					}
					case OP_TST_EQU:{
						if (ip_set_pts_and_test_equal(&test))
						{
							printf("%sError: Point test \"are pts equal?\" on hardware triggered an error.%s\n\r", KERR, KNRM);
							print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TRUE_OR_FALSE')", __LINE__);
						}
						break;
					}
					case OP_TST_OPP:{
						if (ip_set_pts_and_test_oppos(&test))
						{
							printf("%sError: Point test \"are pts opposite?\" on hardware triggered an error.%s\n\r", KERR, KNRM);
							print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TRUE_OR_FALSE')", __LINE__);
						}
						break;
					}
					default:{
						printf("%sError: Invalid test type.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TRUE_OR_FALSE')", __LINE__);
						break;
					}
				}
				/*
				 * Check IP answer to the test against the expected one.
				 */
				switch (test.op) {
					case OP_TST_CHK:{
						if (check_test_oncurve(&test, &stats, &result_tests_are_identical))
						{
							printf("%sError: Couldn't compare hardware result to test \"is on curve?\" "
									"w/ the expected one.%s\n\r", KERR, KNRM);
							print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TRUE_OR_FALSE')", __LINE__);
						}
						break;
					}
					case OP_TST_EQU:{
						if (check_test_equal(&test, &stats, &result_tests_are_identical))
						{
							printf("%sError: Couldn't compare hardware result to test \"are pts equal?\" "
									"w/ the expected one.%s\n\r", KERR, KNRM);
							print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TRUE_OR_FALSE')", __LINE__);
						}
						break;
					}
					case OP_TST_OPP:{
						if (check_test_oppos(&test, &stats, &result_tests_are_identical))
						{
							printf("%sError: Couldn't compare hardware result to test \"are pts opposite?\" "
									"w/ the expected one.%s\n\r", KERR, KNRM);
							print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TRUE_OR_FALSE')", __LINE__);
						}
						break;
					}
					default:{
						printf("%sError: Invalid test type.%s\n\r", KERR, KNRM);
						print_stats_and_exit(&test, &stats, "(debug info: in state 'EXPECT_TRUE_OR_FALSE')", __LINE__);
						break;
					}
				}
				stats.total++;
				line_type_expected = EXPECT_NONE;
				print_stats_regularly(&stats);
#if 0
				/*
				 * Mark the next test to come as not being an exception (a priori)
				 * so that [k]P duration statistics only consider [k]P computations
				 * with no exception
				 */
				test.is_an_exception = false;
#endif
				break;
			}

			default:{
				break;
			}
		} /* switch type of line */

		if (line_type_expected == EXPECT_NONE) {
			/*
			 * Reset a certain number of flags.
			 */
			test.ptp.valid = false;
			test.ptq.valid = false;
			test.pt_sw_res.valid = false;
			test.pt_hw_res.valid = false;
			test.sw_answer.valid = false;
			test.hw_answer.valid = false;
			test.k.valid = false;
			test.blinding = 0;
			test.op = OP_NONE;
			test.is_an_exception = false;
		}

	} /* while nread */

	return EXIT_SUCCESS;
}