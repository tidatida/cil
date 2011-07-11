/*
 * Copyright 2011 Tresys Technology, LLC. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright notice,
 *       this list of conditions and the following disclaimer in the documentation
 *       and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY TRESYS TECHNOLOGY, LLC ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL TRESYS TECHNOLOGY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of Tresys Technology, LLC.
 */

#include <sepol/policydb/policydb.h>

#include "CuTest.h"
#include "CilTest.h"

#include "../../src/cil_build_ast.h"
#include "../../src/cil_resolve_ast.h"

/* this all needs to be moved to a private header file */
int __cil_verify_order(struct cil_list *order, struct cil_tree_node *current, uint32_t flavor);
int __cil_resolve_ast_node_helper(struct cil_tree_node *, uint32_t *, void *);
int __cil_disable_children_helper(struct cil_tree_node *node, __attribute__((unused)) uint32_t *finished, void *);

struct cil_args_resolve {
	struct cil_db *db;
	uint32_t *pass;
	uint32_t *changed;
	struct cil_tree_node *callstack;
	struct cil_tree_node *optstack;
};

struct cil_args_resolve *gen_resolve_args(struct cil_db *db, uint32_t *pass, uint32_t *changed, struct cil_tree_node *calls, struct cil_tree_node *opts)
{
	struct cil_args_resolve *args = cil_malloc(sizeof(*args));
	args->db = db;
	args->pass = pass;
	args->changed = changed;
	args->callstack = calls;
	args->optstack = opts;

	return args;
}

void test_cil_resolve_name(CuTest *tc) {
	char *line[] = { "(", "block", "foo", 
				"(", "typealias", "test", "type_t", ")", 
				"(", "type", "test", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	struct cil_tree_node *test_curr = test_db->ast->root->cl_head->cl_head;
	struct cil_typealias *test_alias = (struct cil_typealias*)test_curr->data;
	struct cil_tree_node *type_node = NULL;

	int rc = cil_resolve_name(test_db, test_curr, test_alias->type_str, CIL_SYM_TYPES, NULL, &type_node);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_name_invalid_type_neg(CuTest *tc) {
	char *line[] = { "(", "block", "foo", 
				"(", "typealias", "foo.test2", "type_t", ")", 
				"(", "type", "test", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	struct cil_tree_node *test_curr = test_db->ast->root->cl_head->cl_head;
	struct cil_typealias *test_alias = (struct cil_typealias*)test_curr->data;
	struct cil_tree_node *type_node = NULL;

	int rc = cil_resolve_name(test_db, test_curr, test_alias->type_str, CIL_SYM_TYPES, NULL, &type_node);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_ast_curr_null_neg(CuTest *tc) {
	struct cil_db *test_db;
	cil_db_init(&test_db);

	test_db->ast->root = NULL;

	int rc = cil_resolve_ast(test_db, test_db->ast->root);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}


/*
	cil_resolve test cases
*/

void test_cil_resolve_roleallow(CuTest *tc) {
	char *line[] = {"(", "role", "foo", ")", \
			"(", "role", "bar", ")", \
			"(", "roleallow", "foo", "bar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roleallow(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_roleallow_srcdecl_neg(CuTest *tc) {
	char *line[] = {"(", "role", "bar", ")", \
			"(", "roleallow", "foo", "bar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	int rc1=cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	rc1 = rc1;

	int rc = cil_resolve_roleallow(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_roleallow_tgtdecl_neg(CuTest *tc) {
	char *line[] = {"(", "role", "foo", ")", \
			"(", "roleallow", "foo", "bar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roleallow(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_roledominance(CuTest *tc) {
	char *line[] = {"(", "role", "foo", ")", \
			"(", "role", "bar", ")", \
			"(", "roledominance", "foo", "bar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roledominance(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_roledominance_role1_neg(CuTest *tc) {
	char *line[] = {"(", "role", "foo", ")", \
			"(", "roledominance", "foo", "bar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roledominance(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_roledominance_role2_neg(CuTest *tc) {
	char *line[] = {"(", "role", "bar", ")", \
			"(", "roledominance", "foo", "bar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roledominance(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_rolebounds(CuTest *tc) {
	char *line[] = {"(", "role", "role1", ")", 
			"(", "role", "role2", ")", 
			"(", "rolebounds", "role1", "role2", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_rolebounds(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_rolebounds_exists_neg(CuTest *tc) {
	char *line[] = {"(", "role", "role1", ")", 
			"(", "role", "role2", ")", 
			"(", "rolebounds", "role1", "role2", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_rolebounds(test_db, test_db->ast->root->cl_head->next->next, NULL);
	int rc = cil_resolve_rolebounds(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_rolebounds_role1_neg(CuTest *tc) {
	char *line[] = {"(", "role", "role1", ")", 
			"(", "role", "role2", ")", 
			"(", "rolebounds", "role_DNE", "role2", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_rolebounds(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_rolebounds_role2_neg(CuTest *tc) {
	char *line[] = {"(", "role", "role1", ")", 
			"(", "role", "role2", ")", 
			"(", "rolebounds", "role1", "role_DNE", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_rolebounds(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_sensalias(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "sensitivityalias", "s0", "alias", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_sensalias(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_sensalias_sensdecl_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivityalias", "s0", "alias", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_sensalias(test_db, test_db->ast->root->cl_head, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_catalias(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")", 
			"(", "categoryalias", "c0", "red", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_catalias(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_catalias_catdecl_neg(CuTest *tc) {
	char *line[] = {"(", "categoryalias", "c0", "red", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_catalias(test_db, test_db->ast->root->cl_head, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);	
}

void test_cil_resolve_catorder(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")", 
			"(", "category", "c3", ")",
			"(", "categoryorder", "(", "c0", "c3", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_catorder(test_db, test_db->ast->root->cl_head->next->next, NULL);
	int rc2 = cil_resolve_catorder(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, SEPOL_OK, rc2);
}

void test_cil_resolve_catorder_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")", 
			"(", "category", "c3", ")",
			"(", "categoryorder", "(", "c5", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_catorder(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_dominance(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")", 
			"(", "sensitivity", "s1", ")",
			"(", "sensitivity", "s2", ")",
			"(", "dominance", "(", "s0", "s1", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_dominance(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc2 = cil_resolve_dominance(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, SEPOL_OK, rc2);
}

void test_cil_resolve_dominance_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")", 
			"(", "sensitivity", "s1", ")",
			"(", "sensitivity", "s2", ")",
			"(", "dominance", "(", "s6", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_dominance(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_cat_list(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryset", "somecats", "(", "c0", "c1", "c2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	struct cil_list *test_cat_list;
	cil_list_init(&test_cat_list);

	struct cil_catset *test_catset = (struct cil_catset*)test_db->ast->root->cl_head->next->next->next->data;

	int rc = cil_resolve_cat_list(test_db, test_db->ast->root->cl_head->next->next->next, test_catset->cat_list_str, test_cat_list, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_cat_list_catlistnull_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryset", "somecats", "(", "c0", "c1", "c2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	struct cil_list *test_cat_list;
	cil_list_init(&test_cat_list);

	struct cil_catset *test_catset = (struct cil_catset*)test_db->ast->root->cl_head->next->next->next->data;
	test_catset->cat_list_str = NULL;

	int rc = cil_resolve_cat_list(test_db, test_db->ast->root->cl_head->next->next->next, test_catset->cat_list_str, test_cat_list, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_cat_list_rescatlistnull_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryset", "somecats", "(", "c0", "c1", "c2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	struct cil_list *test_cat_list = NULL;

	struct cil_catset *test_catset = (struct cil_catset*)test_db->ast->root->cl_head->next->next->next->data;

	int rc = cil_resolve_cat_list(test_db, test_db->ast->root->cl_head->next->next->next, test_catset->cat_list_str, test_cat_list, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_cat_list_catrange(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryorder", "(", "c0", "c1", "c2", ")", ")",
			"(", "categoryset", "somecats", "(", "c0", "(", "c1", "c2", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	uint32_t pass = 4;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_tree_walk(test_db->ast->root, __cil_resolve_ast_node_helper, NULL, NULL, extra_args);
	__cil_verify_order(test_db->catorder, test_db->ast->root, CIL_CAT);

	struct cil_list *test_cat_list;
	cil_list_init(&test_cat_list);

	struct cil_catset *test_catset = (struct cil_catset*)test_db->ast->root->cl_head->next->next->next->next->data;

	int rc = cil_resolve_cat_list(test_db, test_db->ast->root->cl_head->next->next->next->next, test_catset->cat_list_str, test_cat_list, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_cat_list_catrange_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryorder", "(", "c0", "c1", "c2", ")", ")",
			"(", "categoryset", "somecats", "(", "c0", "(", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	struct cil_list *test_cat_list;
	cil_list_init(&test_cat_list);

	struct cil_catset *test_catset = (struct cil_catset*)test_db->ast->root->cl_head->next->next->next->next->data;

	int rc = cil_resolve_cat_list(test_db, test_db->ast->root->cl_head->next->next->next->next, test_catset->cat_list_str, test_cat_list, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_cat_list_catname_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c5", ")",
			"(", "category", "c6", ")",
			"(", "category", "c7", ")",
			"(", "categoryorder", "(", "c0", "c1", "c2", ")", ")",
			"(", "categoryset", "somecats", "(", "c0", "(", "c1", "c2", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	struct cil_list *test_cat_list;
	cil_list_init(&test_cat_list);

	struct cil_catset *test_catset = (struct cil_catset*)test_db->ast->root->cl_head->next->next->next->next->data;
	
	int rc = cil_resolve_cat_list(test_db, test_db->ast->root->cl_head->next->next->next->next, test_catset->cat_list_str, test_cat_list, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_catset(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryset", "somecats", "(", "c0", "c1", "c2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_catset *test_catset = (struct cil_catset *)test_db->ast->root->cl_head->next->next->next->data;
	
	int rc = cil_resolve_catset(test_db, test_db->ast->root->cl_head->next->next->next, test_catset, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_catset_catlist_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryset", "somecats", "(", "c0", "c1", "c2", "c4", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_catset *test_catset = (struct cil_catset *)test_db->ast->root->cl_head->next->next->next->data;
	
	int rc = cil_resolve_catset(test_db, test_db->ast->root->cl_head->next->next->next, test_catset, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_senscat(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
                        "(", "sensitivity", "s1", ")",
                        "(", "dominance", "(", "s0", "s1", ")", ")",
                        "(", "category", "c0", ")",
                        "(", "category", "c255", ")",
                        "(", "categoryorder", "(", "c0", "c255", ")", ")",
                        "(", "sensitivitycategory", "s1", "(", "c0", "c255", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_senscat_catrange_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
                        "(", "sensitivity", "s1", ")",
                        "(", "dominance", "(", "s0", "s1", ")", ")",
                        "(", "category", "c0", ")",
                        "(", "category", "c255", ")",
                        "(", "category", "c500", ")",
                        "(", "categoryorder", "(", "c0", "c255", "c500", ")", ")",
                        "(", "sensitivitycategory", "s1", "(", "c0", "(", "c255", "c5", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_senscat_catsetname(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
                        "(", "sensitivity", "s1", ")",
                        "(", "category", "c0", ")",
                        "(", "category", "c255", ")",
                        "(", "category", "c500", ")",
			"(", "categoryset", "foo", "(", "c0", "c255", "c500", ")", ")",
                        "(", "categoryorder", "(", "c0", "c255", "c500", ")", ")",
                        "(", "sensitivitycategory", "s1", "foo", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_catset *test_catset = (struct cil_catset *)test_db->ast->root->cl_head->next->next->next->next->next->data;
	cil_resolve_catset(test_db, test_db->ast->root->cl_head->next->next->next->next->next, test_catset, NULL);
	int rc = cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_senscat_catsetname_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
                        "(", "sensitivity", "s1", ")",
                        "(", "category", "c0", ")",
                        "(", "category", "c255", ")",
                        "(", "category", "c500", ")",
                        "(", "sensitivitycategory", "s1", "foo", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_senscat_sublist(CuTest *tc) {
      char *line[] = {"(", "sensitivity", "s0", ")",
                        "(", "sensitivity", "s1", ")",
                        "(", "dominance", "(", "s0", "s1", ")", ")",
                        "(", "category", "c0", ")",
                        "(", "category", "c1", ")",
                        "(", "category", "c255", ")",
                        "(", "categoryorder", "(", "c0", "c255", ")", ")",
                        "(", "sensitivitycategory", "s1", "(", "c0", "(", "c1", "c255", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t pass = 4;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_tree_walk(test_db->ast->root, __cil_resolve_ast_node_helper, NULL, NULL, extra_args);
	__cil_verify_order(test_db->catorder, test_db->ast->root, CIL_CAT);

	int rc = cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_senscat_missingsens_neg(CuTest *tc) {
      char *line[] = {"(", "sensitivity", "s0", ")",
                        "(", "dominance", "(", "s0", "s1", ")", ")",
                        "(", "category", "c0", ")",
                        "(", "category", "c255", ")",
                        "(", "categoryorder", "(", "c0", "c255", ")", ")",
                        "(", "sensitivitycategory", "s1", "(", "c0", "c255", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_senscat_category_neg(CuTest *tc) {
      char *line[] = {"(", "sensitivity", "s0", ")",
                        "(", "sensitivity", "s1", ")",
                        "(", "dominance", "(", "s0", "s1", ")", ")",
                        "(", "category", "c0", ")",
                        "(", "category", "c255", ")",
                        "(", "categoryorder", "(", "c0", "c255", ")", ")",
                        "(", "sensitivitycategory", "s1", "(", "c5", "c255", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_senscat_currrangecat(CuTest *tc) {
      char *line[] = {"(", "sensitivity", "s0", ")",
                        "(", "sensitivity", "s1", ")",
                        "(", "dominance", "(", "s0", "s1", ")", ")",
                        "(", "category", "c0", ")",
                        "(", "category", "c1", ")",
                        "(", "category", "c255", ")",
                        "(", "categoryorder", "(", "c0", "c1", "c255", ")", ")",
                        "(", "sensitivitycategory", "s1", "(", "c0", "(", "c1", "c255", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	uint32_t pass = 4;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_tree_walk(test_db->ast->root, __cil_resolve_ast_node_helper, NULL, NULL, extra_args);
	__cil_verify_order(test_db->catorder, test_db->ast->root, CIL_CAT);
	__cil_verify_order(test_db->dominance, test_db->ast->root, CIL_SENS);

	int rc = cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_senscat_currrangecat_neg(CuTest *tc) {
      char *line[] = {"(", "sensitivity", "s0", ")",
                        "(", "sensitivity", "s1", ")",
                        "(", "dominance", "(", "s0", "s1", ")", ")",
                        "(", "category", "c0", ")",
                        "(", "category", "c1", ")",
                        "(", "category", "c255", ")",
                        "(", "categoryorder", "(", "c0", "c1", "c255", ")", ")",
                        "(", "sensitivitycategory", "s1", "(", "c1", "(", "c1", "c255", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	uint32_t pass = 4;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_tree_walk(test_db->ast->root, __cil_resolve_ast_node_helper, NULL, NULL, extra_args);
	__cil_verify_order(test_db->catorder, test_db->ast->root, CIL_CAT);
	__cil_verify_order(test_db->dominance, test_db->ast->root, CIL_SENS);

	int rc = cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_EEXIST, rc);
}

void test_cil_resolve_level(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "categoryorder", "(", "c0", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "type", "blah_t", ")",
			"(", "role", "blah_r", ")",
			"(", "user", "blah_u", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "sidcontext", "test", "(", "blah_u", "blah_r", "blah_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	
	struct cil_tree_node *level = test_db->ast->root->cl_head->next->next->next->next->next->next->next;
	int rc = cil_resolve_level(test_db, level, (struct cil_level*)level->data, NULL);
	int rc2 = cil_resolve_level(test_db, level->next, (struct cil_level*)level->next->data, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, SEPOL_OK, rc2);
}

void test_cil_resolve_level_catlist(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "categoryorder", "(", "c0", "c1", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", "c1", ")", ")",
			"(", "type", "blah_t", ")",
			"(", "role", "blah_r", ")",
			"(", "user", "blah_u", ")",
			"(", "level", "low", "(", "s0", "(", "c0", "c1", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", "c1", ")", ")", ")",
			"(", "sidcontext", "test", "(", "blah_u", "blah_r", "blah_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	
	struct cil_tree_node *level = test_db->ast->root->cl_head->next->next->next->next->next->next->next->next;
	int rc = cil_resolve_level(test_db, level, (struct cil_level*)level->data, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	rc = cil_resolve_level(test_db, level, (struct cil_level*)level->data, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_level_catset(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryset", "cats", "(", "c0", "c1", "c2", ")", ")",	
			"(", "categoryorder", "(", "c0", "c1", "c2", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "sensitivitycategory", "s0", "cats", ")",
			"(", "type", "blah_t", ")",
			"(", "role", "blah_r", ")",
			"(", "user", "blah_u", ")",
			"(", "level", "low", "(", "s0", "cats", ")", ")",
			"(", "level", "high", "(", "s0", "(", "cats", ")", ")", ")",
			"(", "sidcontext", "test", "(", "blah_u", "blah_r", "blah_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_catset *cs = (struct cil_catset *)test_db->ast->root->cl_head->next->next->next->data;

	cil_resolve_catset(test_db, test_db->ast->root->cl_head->next->next->next, cs, NULL);
	cil_resolve_catorder(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next, NULL);
	
	struct cil_tree_node *level = test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next;
	int rc = cil_resolve_level(test_db, level, (struct cil_level*)level->data, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_level_catset_verifysenscat_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryset", "cats", "(", "c0", "c1", "c2", ")", ")",	
			"(", "categoryorder", "(", "c0", "c1", "c2", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "sensitivitycategory", "s0", "cats", ")",
			"(", "type", "blah_t", ")",
			"(", "role", "blah_r", ")",
			"(", "user", "blah_u", ")",
			"(", "level", "low", "(", "s0", "cats", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "sidcontext", "test", "(", "blah_u", "blah_r", "blah_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	
	struct cil_tree_node *level = test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next;
	int rc = cil_resolve_level(test_db, level, (struct cil_level*)level->data, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_level_catset_name_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryset", "cats", "(", "c0", "c1", "c2", ")", ")",	
			"(", "categoryorder", "(", "c0", "c1", "c2", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "sensitivitycategory", "s0", "cats", ")",
			"(", "type", "blah_t", ")",
			"(", "role", "blah_r", ")",
			"(", "user", "blah_u", ")",
			"(", "level", "low", "(", "s0", "dne", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "sidcontext", "test", "(", "blah_u", "blah_r", "blah_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	
	struct cil_tree_node *level = test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next;
	int rc = cil_resolve_level(test_db, level, (struct cil_level*)level->data, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_level_catset_resolvecatset_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryset", "cats", "(", "c0", "c1", "c2", "c4", ")", ")",	
			"(", "categoryorder", "(", "c0", "c1", "c2", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "sensitivitycategory", "s0", "cats", ")",
			"(", "type", "blah_t", ")",
			"(", "role", "blah_r", ")",
			"(", "user", "blah_u", ")",
			"(", "level", "low", "(", "s0", "cats", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "sidcontext", "test", "(", "blah_u", "blah_r", "blah_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	
	struct cil_tree_node *level = test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next;
	int rc = cil_resolve_level(test_db, level, (struct cil_level*)level->data, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_level_sens_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "categoryorder", "(", "c0", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "type", "blah_t", ")",
			"(", "role", "blah_r", ")",
			"(", "user", "blah_u", ")",
			"(", "level", "low", "(", "s1", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s1", "(", "c0", ")", ")", ")",
			"(", "sid", "test", "(", "blah_u", "blah_r", "blah_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	struct cil_tree_node *level = test_db->ast->root->cl_head->next->next->next->next->next->next->next;
	int rc = cil_resolve_level(test_db, level, (struct cil_level*)level->data, NULL);
	int rc2 = cil_resolve_level(test_db, level->next, (struct cil_level*)level->next->data, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc2);
}

void test_cil_resolve_level_cat_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "categoryorder", "(", "c0", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "type", "blah_t", ")",
			"(", "role", "blah_r", ")",
			"(", "user", "blah_u", ")",
			"(", "level", "low", "(", "s0", "(", "c1", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c1", ")", ")", ")",
			"(", "sid", "test", "(", "blah_u", "blah_r", "blah_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	struct cil_tree_node *level = test_db->ast->root->cl_head->next->next->next->next->next->next->next;
	int rc = cil_resolve_level(test_db, level, (struct cil_level*)level->data, NULL);
	int rc2 = cil_resolve_level(test_db, level->next, (struct cil_level*)level->next->data, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc2);
}

void test_cil_resolve_level_senscat_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "categoryorder", "(", "c0", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "sensitivitycategory", "s1", "(", "c0", ")", ")",
			"(", "type", "blah_t", ")",
			"(", "role", "blah_r", ")",
			"(", "user", "blah_u", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "sid", "test", "(", "blah_u", "blah_r", "blah_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	struct cil_tree_node *level = test_db->ast->root->cl_head->next->next->next->next->next->next->next;
	int rc = cil_resolve_level(test_db, level, (struct cil_level*)level->data, NULL);
	int rc2 = cil_resolve_level(test_db, level->next, (struct cil_level*)level->next->data, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc2);
}

void test_cil_resolve_constrain(CuTest *tc) {
        char *line[] = {"(", "class", "file", "(", "create", "relabelto", ")", ")",
			"(", "class", "dir", "(", "create", "relabelto", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c1", ")",
			"(", "level", "l2", "(", "s0", "(", "c1", ")", ")", ")",
			"(", "level", "h2", "(", "s0", "(", "c1", ")", ")", ")",
			"(", "mlsconstrain", "(", "file", "dir", ")", "(", "create", "relabelto", ")", "(", "eq", "l2", "h2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_constrain(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_constrain_class_neg(CuTest *tc) {
        char *line[] = {"(", "class", "file", "(", "create", "relabelto", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c1", ")",
			"(", "level", "l2", "(", "s0", "(", "c1", ")", ")", ")",
			"(", "level", "h2", "(", "s0", "(", "c1", ")", ")", ")",
			"(", "mlsconstrain", "(", "file", "dir", ")", "(", "create", "relabelto", ")", "(", "eq", "l2", "h2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_constrain(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_constrain_perm_neg(CuTest *tc) {
        char *line[] = {"(", "class", "file", "(", "create", ")", ")",
			"(", "class", "dir", "(", "create", "relabelto", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c1", ")",
			"(", "level", "l2", "(", "s0", "(", "c1", ")", ")", ")",
			"(", "level", "h2", "(", "s0", "(", "c1", ")", ")", ")",
			"(", "mlsconstrain", "(", "file", "dir", ")", "(", "create", "relabelto", ")", "(", "eq", "l2", "h2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_constrain(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_constrain_perm_resolve_neg(CuTest *tc) {
        char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c1", ")",
			"(", "level", "l2", "(", "s0", "(", "c1", ")", ")", ")",
			"(", "level", "h2", "(", "s0", "(", "c1", ")", ")", ")",
			"(", "mlsconstrain", "(", "file", "dir", ")", "(", "foo", ")", "(", "eq", "l2", "h2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_constrain(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_context(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con",
                        "(", "system_u", "object_r", "netif_t", "low", "high", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_context *test_context = (struct cil_context*)test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->data;

	int rc = cil_resolve_context(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, test_context, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_context_user_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con",
                        "(", "system_u", "object_r", "netif_t", "low", "high", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_context *test_context = (struct cil_context*)test_db->ast->root->cl_head->next->next->next->next->next->next->next->data;

	int rc = cil_resolve_context(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, test_context, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_context_role_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con",
                        "(", "system_u", "object_r", "netif_t", "low", "high", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_context *test_context = (struct cil_context*)test_db->ast->root->cl_head->next->next->next->next->next->next->next->data;

	int rc = cil_resolve_context(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, test_context, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_context_type_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "context", "con",
                        "(", "system_u", "object_r", "netif_t", "low", "high", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_context *test_context = (struct cil_context*)test_db->ast->root->cl_head->next->next->next->next->next->next->next->data;

	int rc = cil_resolve_context(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, test_context, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_context_low_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con",
                        "(", "system_u", "object_r", "netif_t", "lowl", "high", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_context *test_context = (struct cil_context*)test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->data;

	int rc = cil_resolve_context(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, test_context, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_context_high_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con",
                        "(", "system_u", "object_r", "netif_t", "low", "sdhigh", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_context *test_context = (struct cil_context*)test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->data;

	int rc = cil_resolve_context(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, test_context, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_context_low_unnamed_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con",
                        "(", "system_u", "object_r", "netif_t", "(", "s0", "(", "c0", ")", ")", "high", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_context *test_context = (struct cil_context*)test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->data;

	int rc = cil_resolve_context(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, test_context, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_context_high_unnamed_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con",
                        "(", "system_u", "object_r", "netif_t", "high", "(", "s0", "(", "c0", ")", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_context *test_context = (struct cil_context*)test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->data;

	int rc = cil_resolve_context(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, test_context, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_context_lownull_unnamed_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con",
                        "(", "system_u", "object_r", "netif_t", "low", "high", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_context *test_context = (struct cil_context*)test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->data;
	test_context->low_str = NULL;
	test_context->low = NULL;

	int rc = cil_resolve_context(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, test_context, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_context_highnull_unnamed_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con",
                        "(", "system_u", "object_r", "netif_t", "low", "high", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_context *test_context = (struct cil_context*)test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->data;
	test_context->high_str = NULL;
	test_context->high = NULL;
	
	int rc = cil_resolve_context(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, test_context, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_roletrans(CuTest *tc) {
	char *line[] = {"(", "role", "foo_r", ")",
			"(", "type", "bar_t", ")",
			"(", "role", "foobar_r", ")",
			"(", "class", "process", "(", "transition", ")", ")",
			"(", "roletransition", "foo_r", "bar_t", "process", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roletrans(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_roletrans_srcdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "bar_t", ")",
			"(", "role", "foobar_r", ")", 
			"(", "class", "process", "(", "transition", ")", ")",
			"(", "roletransition", "foo_r", "bar_t", "process", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roletrans(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_roletrans_tgtdecl_neg(CuTest *tc) {
	char *line[] = {"(", "role", "foo_r", ")",
			"(", "role", "foobar_r", ")", 
			"(", "class", "process", "(", "transition", ")", ")",
			"(", "roletransition", "foo_r", "bar_t", "process", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roletrans(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_roletrans_resultdecl_neg(CuTest *tc) {
	char *line[] = {"(", "role", "foo_r", ")",
			"(", "type", "bar_t", ")", 
			"(", "class", "process", "(", "transition", ")", ")",
			"(", "roletransition", "foo_r", "bar_t", "process", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roletrans(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_attrtypes_type_in_multiple_attrs(CuTest *tc) {
	char *line[] = {"(", "attribute", "attrs", ")",
			"(", "attribute", "attrs2", ")",
			"(", "type", "type_t", ")",
			"(", "attributetypes", "attrs2", "(", "type_t", ")", ")",
			"(", "attributetypes", "attrs", "(", "type_t", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_attrtypes(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	int rc2 = cil_resolve_attrtypes(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, SEPOL_OK, rc2);
}

void test_cil_resolve_attrtypes_multiple_excludes(CuTest *tc) {
	char *line[] = {"(", "attribute", "attrs", ")",
			"(", "type", "type_t", ")",
			"(", "type", "type_b", ")",
			"(", "type", "type_a", ")",
			"(", "attributetypes", "attrs", "(", "type_t", "-type_a", "-type_b", ")", ")", NULL}; 

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_attrtypes(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_attrtypes_multiple_types(CuTest *tc) {
	char *line[] = {"(", "attribute", "attrs", ")",
			"(", "type", "type_t", ")",
			"(", "type", "type_tt", ")",
			"(", "attributetypes", "attrs", "(", "type_t", "type_tt", ")", ")", NULL}; 

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_attrtypes(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_attrtypes_list_of_attrs(CuTest *tc) {
	char *line[] = {"(", "attribute", "attrs", ")",
			"(", "attribute", "attr_a", ")",
			"(", "attributetypes", "attrs", "(", "attr_a", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_attrtypes(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_attrtypes_name_neg(CuTest *tc) {
	char *line[] = {"(", "type", "type_t", ")",
			"(", "attributetypes", "attrs", "(", "type_t", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_attrtypes(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_attrtypes_list_neg(CuTest *tc) {
	char *line[] = {"(", "attribute", "attrs", ")",
			"(", "attributetypes", "attrs", "(", "type_t", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_attrtypes(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_attrtypes_exclude(CuTest *tc) {
	char *line[] = {"(", "attribute", "attrs", ")",
			"(", "type", "type_t", ")",
			"(", "type", "t_t", ")",
			"(", "attributetypes", "attrs", "(", "type_t", "-t_t", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_attrtypes(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_attrtypes_exclude_neg(CuTest *tc) {
	char *line[] = {"(", "attribute", "attrs", ")",
			"(", "type", "type_t", ")",
			"(", "attributetypes", "attrs", "(", "type_t", "-t_t", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_attrtypes(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_typealias(CuTest *tc) {
	char *line[] = {"(", "block", "foo", 
				"(", "typealias", ".foo.test", "type_t", ")", 
				"(", "type", "test", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);


	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_typealias(test_db, test_db->ast->root->cl_head->cl_head, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_typealias_neg(CuTest *tc) {
	char *line[] = {"(", "block", "foo", 
				"(", "typealias", ".foo", "apache_alias", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_typealias(test_db, test_db->ast->root->cl_head->cl_head, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_typebounds(CuTest *tc) {
	char *line[] = {"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "typebounds", "type_a", "type_b", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);


	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_typebounds(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_typebounds_type1_neg(CuTest *tc) {
	char *line[] = {"(", "type", "type_b", ")",
			"(", "typebounds", "type_a", "type_b", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_typebounds(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_typebounds_type2_neg(CuTest *tc) {
	char *line[] = {"(", "type", "type_a", ")",
			"(", "typebounds", "type_a", "type_b", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_typebounds(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_typepermissive(CuTest *tc) {
	char *line[] = {"(", "type", "type_a", ")",
			"(", "typepermissive", "type_a", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);


	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_typepermissive(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_typepermissive_neg(CuTest *tc) {
	char *line[] = {"(", "type", "type_a", ")",
			"(", "typepermissive", "type_b", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);


	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_typepermissive(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_filetransition(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "type", "foobar", ")",
			"(", "filetransition", "foo", "bar", "file", "foobar", "str", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);


	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_filetransition(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_filetransition_type1_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "type", "foobar", ")",
			"(", "filetransition", "wrong", "bar", "file", "foobar", "str", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);


	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_filetransition(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_filetransition_type2_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "type", "foobar", ")",
			"(", "filetransition", "foo", "wrong", "file", "foobar", "str", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);


	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_filetransition(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_filetransition_class_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "type", "foobar", ")",
			"(", "filetransition", "foo", "bar", "wrong", "foobar", "str", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);


	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_filetransition(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_filetransition_type3_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "type", "foobar", ")",
			"(", "filetransition", "foo", "bar", "file", "wrong", "str", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_filetransition(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_rangetransition(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "rangetransition", "type_a", "type_b", "class_", "low", "high", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_rangetransition(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_rangetransition_type1_neg(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "rangetransition", "type_DNE", "type_b", "class_", "low", "high", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_rangetransition(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_rangetransition_type2_neg(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "rangetransition", "type_a", "type_DNE", "class_", "low", "high", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_rangetransition(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_rangetransition_class_neg(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "rangetransition", "type_a", "type_b", "class_DNE", "low", "high", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_rangetransition(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_rangetransition_call_level_l_anon(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "macro", "mm", "(", "(", "level", "l", ")", ")",
				"(", "rangetransition", "type_a", "type_b", "class_", "l", "high", ")", ")",
			"(", "call", "mm", "(", "(", "s0", "(", "c0", ")", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	int rc2 = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	int rc3 = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	int rc = cil_resolve_rangetransition(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->cl_head, (struct cil_call*)test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->data);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, SEPOL_OK, rc2);
	CuAssertIntEquals(tc, SEPOL_OK, rc3);
}

void test_cil_resolve_rangetransition_call_level_l_anon_neg(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "macro", "mm", "(", "(", "level", "l", ")", ")",
				"(", "rangetransition", "type_a", "type_b", "class_", "l", "high", ")", ")",
			"(", "call", "mm", "(", "(", "s0", "(", "c4", ")", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	int rc2 = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	int rc3 = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	int rc = cil_resolve_rangetransition(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->cl_head, (struct cil_call*)test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->data);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, SEPOL_OK, rc2);
	CuAssertIntEquals(tc, SEPOL_OK, rc3);
}

void test_cil_resolve_rangetransition_call_level_h_anon(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "macro", "mm", "(", "(", "level", "h", ")", ")",
				"(", "rangetransition", "type_a", "type_b", "class_", "low", "h", ")", ")",
			"(", "call", "mm", "(", "(", "s0", "(", "c0", ")", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	int rc2 = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	int rc3 = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	int rc = cil_resolve_rangetransition(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->cl_head, (struct cil_call*)test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->data);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, SEPOL_OK, rc2);
	CuAssertIntEquals(tc, SEPOL_OK, rc3);
}

void test_cil_resolve_rangetransition_call_level_h_anon_neg(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "macro", "mm", "(", "(", "level", "h", ")", ")",
				"(", "rangetransition", "type_a", "type_b", "class_", "low", "h", ")", ")",
			"(", "call", "mm", "(", "(", "s0", "(", "c4", ")", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	int rc2 = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	int rc3 = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	int rc = cil_resolve_rangetransition(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->cl_head, (struct cil_call*)test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->data);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, SEPOL_OK, rc2);
	CuAssertIntEquals(tc, SEPOL_OK, rc3);
}

void test_cil_resolve_rangetransition_level_l_neg(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "rangetransition", "type_a", "type_b", "class_", "low_DNE", "high", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_rangetransition(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_rangetransition_level_h_neg(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "rangetransition", "type_a", "type_b", "class_", "low", "high_DNE", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_rangetransition(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_rangetransition_anon_level_l(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "rangetransition", "type_a", "type_b", "class_", "(", "s0", "(", "c0", ")", ")", "high", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	int rc = cil_resolve_rangetransition(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_rangetransition_anon_level_l_neg(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "rangetransition", "type_a", "type_b", "class_", "(", "s0", "(", "c_DNE", ")", ")", "high", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	int rc = cil_resolve_rangetransition(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_rangetransition_anon_level_h(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "rangetransition", "type_a", "type_b", "class_", "low", "(", "s0", "(", "c0", ")",  ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	int rc = cil_resolve_rangetransition(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_rangetransition_anon_level_h_neg(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "rangetransition", "type_a", "type_b", "class_", "low", "(", "s_DNE", "(", "c0", ")",  ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	int rc = cil_resolve_rangetransition(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_classcommon(CuTest *tc) {
	char *line[] = {"(", "class", "file", "(", "read", ")", ")",
			"(", "common", "file", "(", "write", ")", ")",	
			"(", "classcommon", "file", "file", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_classcommon(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_classcommon_no_class_neg(CuTest *tc) {
	char *line[] = {"(", "class", "foo", "(", "read", ")", ")",
			"(", "classcommon", "foo", "foo", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_classcommon(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_classcommon_no_common_neg(CuTest *tc) {
	char *line[] = {"(", "common", "foo", "(", "read", ")", ")",
			"(", "classcommon", "foo", "foo", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_classcommon(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_avrule(CuTest *tc) {
	char *line[] = {"(", "class", "bar", "(", "read", "write", "open", ")", ")", 
	                "(", "type", "test", ")", 
			"(", "type", "foo", ")", 
	                "(", "allow", "test", "foo", "bar", "(", "read", "write", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_avrule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_avrule_permset(CuTest *tc) {
	char *line[] = {"(", "class", "bar", "(", "read", "write", "open", ")", ")", 
	                "(", "type", "test", ")", 
			"(", "type", "foo", ")",
			"(", "permissionset", "baz", "(", "open", "write", ")", ")",
	                "(", "allow", "test", "foo", "bar", "baz", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_avrule(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_avrule_permset_neg(CuTest *tc) {
	char *line[] = {"(", "class", "bar", "(", "read", "write", "open", ")", ")", 
	                "(", "type", "test", ")", 
			"(", "type", "foo", ")",
			"(", "permissionset", "baz", "(", "open", "close", ")", ")",
	                "(", "allow", "test", "foo", "bar", "dne", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_avrule(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_avrule_firsttype_neg(CuTest *tc) {
	char *line[] = {"(", "class", "bar", "(", "read", "write", "open", ")", ")", 
	                "(", "type", "test", ")", 
			"(", "type", "foo", ")", 
	                "(", "allow", "fail1", "foo", "bar", "(", "read", "write", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_avrule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_avrule_secondtype_neg(CuTest *tc) {
	char *line[] = {"(", "class", "bar", "(", "read", "write", "open", ")", ")", 
	                "(", "type", "test", ")", 
			"(", "type", "foo", ")", 
	                "(", "allow", "test", "fail2", "bar", "(", "read", "write", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_avrule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_avrule_class_neg(CuTest *tc) {
	char *line[] = {"(", "class", "bar", "(", "read", "write", "open", ")", ")", 
	                "(", "type", "test", ")", 
			"(", "type", "foo", ")", 
	                "(", "allow", "test", "foo", "fail3", "(", "read", "write", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_avrule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_avrule_perm_neg(CuTest *tc) {
	char *line[] = {"(", "class", "bar", "(", "read", "write", "open", ")", ")", 
	                "(", "type", "test", ")", 
			"(", "type", "foo", ")", 
	                "(", "allow", "test", "foo", "bar", "(", "execute", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_avrule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_type_rule_transition(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "type", "foobar", ")", 
			"(", "typetransition", "foo", "bar", "file", "foobar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_type_rule(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_type_rule_transition_srcdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "bar", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "type", "foobar", ")", 
			"(", "typetransition", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_type_rule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_type_rule_transition_tgtdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "type", "foobar", ")", 
			"(", "typetransition", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_type_rule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_type_rule_transition_objdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "type", "foobar", ")", 
			"(", "typetransition", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_type_rule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_type_rule_transition_resultdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "typetransition", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_type_rule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_type_rule_change(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "type", "foobar", ")", 
			"(", "typechange", "foo", "bar", "file", "foobar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_type_rule(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_type_rule_change_srcdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "bar", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "type", "foobar", ")", 
			"(", "typechange", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_type_rule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_type_rule_change_tgtdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "type", "foobar", ")", 
			"(", "typechange", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_type_rule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_type_rule_change_objdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "type", "foobar", ")", 
			"(", "typechange", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_type_rule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_type_rule_change_resultdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "typechange", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_type_rule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_type_rule_member(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "type", "foobar", ")", 
			"(", "typemember", "foo", "bar", "file", "foobar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_type_rule(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_type_rule_member_srcdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "bar", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "type", "foobar", ")", 
			"(", "typemember", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_type_rule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_type_rule_member_tgtdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "type", "foobar", ")", 
			"(", "typemember", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_type_rule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_type_rule_member_objdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "type", "foobar", ")", 
			"(", "typemember", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_type_rule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_type_rule_member_resultdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "typemember", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_type_rule(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_filecon(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "filecon", "root", "path", "file", "con", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_filecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_filecon_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "filecon", "root", "path", "file", "conn", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_filecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_filecon_anon_context(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "filecon", "root", "path", "file", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_filecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_filecon_anon_context_neg(CuTest *tc) {
	char *line[] = {"(", "user", "system_u", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "filecon", "root", "path", "file", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_filecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_portcon(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "portcon", "type", "25", "con", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_portcon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_portcon_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "portcon", "type", "25", "conn", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_portcon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_portcon_anon_context(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "portcon", "type", "25", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_portcon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_portcon_anon_context_neg(CuTest *tc) {
	char *line[] = {"(", "user", "system_u", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "portcon", "type", "25", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_portcon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_genfscon(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "genfscon", "type", "path", "con", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_genfscon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_genfscon_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "genfscon", "type", "path", "conn", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_genfscon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_genfscon_anon_context(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "genfscon", "type", "path", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_genfscon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_genfscon_anon_context_neg(CuTest *tc) {
	char *line[] = {"(", "user", "system_u", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "genfscon", "type", "path", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_genfscon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_nodecon_ipv4(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "ipaddr", "ip", "192.168.1.1", ")",
			"(", "ipaddr", "netmask", "192.168.1.1", ")",
			"(", "nodecon", "ip", "netmask", "con", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_nodecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_nodecon_ipv6(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "ipaddr", "ip", "2001:0DB8:AC10:FE01::", ")",
			"(", "ipaddr", "netmask", "2001:0DB8:AC10:FE01::", ")",
			"(", "nodecon", "ip", "netmask", "con", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_nodecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_nodecon_anonipaddr_ipv4(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "ipaddr", "netmask", "192.168.1.1", ")",
			"(", "nodecon", "(", "192.168.1.1", ")", "netmask", "con", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_nodecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_nodecon_anonnetmask_ipv4(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "ipaddr", "ip", "192.168.1.1", ")",
			"(", "nodecon", "ip", "(", "192.168.1.1", ")", "con", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_nodecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_nodecon_anonipaddr_ipv6(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "ipaddr", "netmask", "2001:0DB8:AC10:FE01::", ")",
			"(", "nodecon", "(", "2001:0DB8:AC10:FE01::", ")", "netmask", "con", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_nodecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_nodecon_anonnetmask_ipv6(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "ipaddr", "ip", "2001:0DB8:AC10:FE01::", ")",
			"(", "nodecon", "ip", "(", "2001:0DB8:AC10:FE01::", ")", "con", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_nodecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_nodecon_diffipfam_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "ipaddr", "ip", "2001:0DB8:AC10:FE01::", ")",
			"(", "nodecon", "ip", "(", "192.168.1.1", ")", "con", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_nodecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_nodecon_context_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "ipaddr", "ip", "192.168.1.1", ")",
			"(", "ipaddr", "netmask", "192.168.1.1", ")",
			"(", "nodecon", "n", "netmask", "con", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_nodecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_nodecon_ipaddr_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "ipaddr", "ip", "192.168.1.1", ")",
			"(", "ipaddr", "netmask", "192.168.1.1", ")",
			"(", "nodecon", "ip", "n", "con", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_nodecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_nodecon_netmask_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "ipaddr", "ip", "192.168.1.1", ")",
			"(", "nodecon", "ip", "ip", "conn", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_nodecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_nodecon_anon_context(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "ipaddr", "ip", "192.168.1.1", ")",
			"(", "nodecon", "ip", "ip", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_nodecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_nodecon_anon_context_neg(CuTest *tc) {
	char *line[] = {"(", "user", "system_u", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "ipaddr", "ip", "192.168.1.1", ")",
			"(", "nodecon", "ip", "ip", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_nodecon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_netifcon(CuTest *tc) {
	char *line[] = {"(", "context", "if_default", "(", "system_u", "object_r", "etc_t", "low", "high", ")", ")",
			"(", "context", "packet_default", "(", "system_u", "object_r", "etc_t", "low", "high", ")", ")", 
			"(", "netifcon", "eth0", "if_default", "packet_default", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_netifcon(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_netifcon_otf_neg(CuTest *tc) {
	char *line[] = {"(", "context", "if_default", "(", "system_u", "object_r", "etc_t", "low", "high", ")", ")",
			"(", "netifcon", "eth0", "if_default", "packet_default", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_netifcon(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_netifcon_interface_neg(CuTest *tc) {
	char *line[] = {"(", "context", "packet_default", "(", "system_u", "object_r", "etc_t", "low", "high", ")", ")", 
			"(", "netifcon", "eth0", "if_default", "packet_default", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_netifcon(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_netifcon_unnamed(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")", 
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "netifcon", "eth1",
                        "(", "system_u", "object_r", "netif_t", "low", "high", ")",
                        "(", "system_u", "object_r", "netif_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_netifcon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_netifcon_unnamed_packet_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")", 
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "netifcon", "eth1",
                        "(", "system_u", "object_r", "netif_t", "low", "high", ")",
                        "(", "system_u", "object_r", "netif_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_netifcon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_netifcon_unnamed_otf_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")", 
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "netifcon", "eth1",
                        "(", "system_u", "object_r", "netif_t", "low", "high", ")", 
                        "(", "system_u", "foo_r", "netif_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_netifcon(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_netifcon_sublist_secondlist_missing_neg(CuTest *tc) {
	char *line[] = {"(", "netifcon", "eth1",
                        "(", "system_u", "object_r", "netif_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_netifcon(test_db, test_db->ast->root->cl_head, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_fsuse(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con", "(", "system_u", "object_r", "netif_t", "low", "high", ")", ")",
			"(", "fsuse", "xattr", "ext3", "con", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_fsuse(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_fsuse_nocontext_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con", "(", "system_u", "object_r", "netif_t", "low", "high", ")", ")",
			"(", "fsuse", "xattr", "ext3", "(", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	int rc1 = cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	rc1 = rc1;

	int rc = cil_resolve_fsuse(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_fsuse_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con", "(", "system_u", "object_r", "netif_t", "low", "high", ")", ")",
			"(", "fsuse", "xattr", "ext3", "conn", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_fsuse(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_fsuse_anon(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "fsuse", "xattr", "ext3", "(", "system_u", "object_r", "netif_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_fsuse(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_fsuse_anon_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "fsuse", "xattr", "ext3", "(", "system_uu", "object_r", "netif_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_fsuse(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_sidcontext(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "categoryorder", "(", "c0", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "type", "blah_t", ")",
			"(", "role", "blah_r", ")",
			"(", "user", "blah_u", ")",
			"(", "sid", "test", ")",
			"(", "sidcontext", "test", "(", "blah_u", "blah_r", "blah_t", "(", "s0", "(", "c0", ")", ")", "(", "s0", "(", "c0", ")", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	
	int rc = cil_resolve_sidcontext(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_sidcontext_named_levels(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "categoryorder", "(", "c0", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "type", "blah_t", ")",
			"(", "role", "blah_r", ")",
			"(", "user", "blah_u", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "sid", "test", ")",
			"(", "sidcontext", "test", "(", "blah_u", "blah_r", "blah_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	struct cil_tree_node *level = test_db->ast->root->cl_head->next->next->next->next->next->next->next;
	cil_resolve_level(test_db, level, (struct cil_level*)level->data, NULL);
	cil_resolve_level(test_db, level->next, (struct cil_level*)level->next->data, NULL);
	int rc = cil_resolve_sidcontext(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_sidcontext_named_context(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "categoryorder", "(", "c0", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "type", "blah_t", ")",
			"(", "role", "blah_r", ")",
			"(", "user", "blah_u", ")",
			"(", "context", "con", "(", "blah_u", "blah_r", "blah_t", "(", "s0", "(", "c0", ")", ")", "(", "s0", "(", "c0", ")", ")", ")", ")",
			"(", "sid", "test", ")",
			"(", "sidcontext", "test", "con", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	struct cil_tree_node *context = test_db->ast->root->cl_head->next->next->next->next->next->next->next;
	cil_resolve_context(test_db, context, (struct cil_context*)context->data, NULL);

	int rc = cil_resolve_sidcontext(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_sidcontext_named_context_wrongname_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "categoryorder", "(", "c0", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "type", "blah_t", ")",
			"(", "role", "blah_r", ")",
			"(", "user", "blah_u", ")",
			"(", "context", "con", "(", "blah_u", "blah_r", "blah_t", "(", "s0", "(", "c0", ")", ")", "(", "s0", "(", "c0", ")", ")", ")", ")",
			"(", "sid", "test", ")",
			"(", "sidcontext", "test", "foo", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	struct cil_tree_node *context = test_db->ast->root->cl_head->next->next->next->next->next->next->next;
	cil_resolve_context(test_db, context, (struct cil_context*)context->data, NULL);
	
	int rc = cil_resolve_sidcontext(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_sidcontext_named_context_invaliduser_neg(CuTest *tc) {
           char *line[] = {"(", "category", "c0", ")",
                        "(", "categoryorder", "(", "c0", ")", ")",
                        "(", "sensitivity", "s0", ")",
                        "(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
                        "(", "type", "blah_t", ")",
                        "(", "role", "blah_r", ")",
                        "(", "user", "blah_u", ")",
			"(", "sid", "test", ")",
                        "(", "sidcontext", "test", "(", "", "blah_r", "blah_t", "(", "s0", "(", "c0", ")", ")", "(", "s0", "(", "c0", ")", ")", ")", ")", NULL};

        struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next, NULL);

        int rc = cil_resolve_sidcontext(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
        CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_call1_noparam(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "qaz", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call1_type(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "qaz", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call1_role(CuTest *tc) {
	char *line[] = {"(", "role", "role_r", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "role", "a", ")", ")",
				"(", "role", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "role_r", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call1_user(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "user", "a", ")", ")",
				"(", "user", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "user_u", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call1_sens(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "sens", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "sensitivity", "a", ")", ")",
				"(", "sensitivity", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "sens", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call1_cat(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "category", "a", ")", ")",
				"(", "category", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "c0", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call1_catset(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryset", "somecats", "(", "c0", "c1", "c2", ")", ")",
			"(", "macro", "mm", "(", "(", "categoryset",  "foo", ")", ")",
				"(", "level", "bar", "(", "s0", "foo", ")", ")", ")",
			"(", "call", "mm", "(", "somecats", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call1_catset_anon(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "macro", "mm", "(", "(", "categoryset",  "foo", ")", ")",
				"(", "level", "bar", "(", "s0", "foo", ")", ")", ")",
			"(", "call", "mm", "(", "(", "c0", "c1", "c2", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call1_catset_anon_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "macro", "mm", "(", "(", "categoryset",  "foo", ")", ")",
				"(", "level", "bar", "(", "s0", "foo", ")", ")", ")",
			"(", "call", "mm", "(", "(", "c5", "(", "c2", ")", "c4", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_call1_level(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "l", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "h", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "macro", "mm", "(", "(", "level", "lvl_l", ")", "(", "level", "lvl_h", ")", ")",
				"(", "context", "foo", "(", "system_u", "role_r", "type_t", "lvl_l", "lvl_h", ")", ")", ")",
			"(", "call", "mm", "(", "l", "h", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call1_level_anon(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "l", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "h", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "macro", "mm", "(", "(", "level", "lvl_l", ")", ")",
				"(", "context", "foo", "(", "system_u", "role_r", "type_t", "lvl_l", "h", ")", ")", ")",
			"(", "call", "mm", "(", "(", "s0", "(", "c0", ")", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call1_level_anon_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "l", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "h", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "macro", "mm", "(", "(", "level", "lvl_l", ")", ")",
				"(", "context", "foo", "(", "system_u", "role_r", "type_t", "lvl_l", "h", ")", ")", ")",
			"(", "call", "mm", "(", "(", "s0", "(", "c0", "(", "c5", ")", ")", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_call1_ipaddr(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "lvl_l", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "lvl_h", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "system_u", "role_r", "type_t", "lvl_l", "lvl_h", ")", ")",
			"(", "ipaddr", "netmask", "192.168.0.1", ")",
			"(", "ipaddr", "ip", "192.168.0.1", ")",
			"(", "macro", "mm", "(", "(", "ipaddr", "addr", ")", ")",
				"(", "nodecon", "addr", "netmask", "con", ")", ")",
			"(", "call", "mm", "(", "ip", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call1_ipaddr_anon(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "lvl_l", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "lvl_h", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "system_u", "role_r", "type_t", "lvl_l", "lvl_h", ")", ")",
			"(", "ipaddr", "netmask", "192.168.0.1", ")",
			"(", "macro", "mm", "(", "(", "ipaddr", "addr", ")", ")",
				"(", "nodecon", "addr", "netmask", "con", ")", ")",
			"(", "call", "mm", "(", "(", "192.168.1.1", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call1_ipaddr_anon_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "lvl_l", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "lvl_h", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "system_u", "role_r", "type_t", "lvl_l", "lvl_h", ")", ")",
			"(", "ipaddr", "netmask", "192.168.0.1", ")",
			"(", "macro", "mm", "(", "(", "ipaddr", "addr", ")", ")",
				"(", "nodecon", "addr", "netmask", "con", ")", ")",
			"(", "call", "mm", "(", "(", "192.1.1", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_call1_class(CuTest *tc) {
	char *line[] = {"(", "class", "foo", "(", "read", ")", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "class", "a", ")", ")",
				"(", "class", "b", "(", "read", ")", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "foo", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call1_permset(CuTest *tc) {
	char *line[] = {"(", "permissionset", "foo", "(", "read", "open", ")", ")",
			"(", "class", "dead", "(", "close", ")", ")",
			"(", "class", "bar", "(", "close", ")", ")",
			"(", "class", "baz", "(", "close", ")", ")",
			"(", "macro", "mm", "(", "(", "permissionset", "a", ")", ")", 
				"(", "allow", "dead", "bar", "baz", "a", ")", ")",
			"(", "call", "mm", "(", "foo", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	int rc1 = cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	rc1 = rc1;

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call1_permset_anon(CuTest *tc) {
	char *line[] = {"(", "class", "dead", "(", "close", ")", ")",
			"(", "class", "bar", "(", "close", ")", ")",
			"(", "class", "baz", "(", "close", ")", ")",
			"(", "macro", "mm", "(", "(", "permissionset", "a", ")", ")", 
				"(", "allow", "dead", "bar", "baz", "a", ")", ")",
			"(", "call", "mm", "(", "(", "read", "open", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	int rc1 = cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	rc1 = rc1;

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call1_unknown_neg(CuTest *tc) {
	char *line[] = {"(", "class", "foo", "(", "read", ")", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "class", "a", ")", ")",
				"(", "class", "b", "(", "read", ")", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "foo", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tree_node *macro_node = NULL;
	cil_resolve_name(test_db, test_db->ast->root->cl_head->next->next->next, ((struct cil_call*)test_db->ast->root->cl_head->next->next->next->data)->macro_str, CIL_SYM_MACROS, NULL, &macro_node);
	((struct cil_call*)test_db->ast->root->cl_head->next->next->next->data)->macro = (struct cil_macro*)macro_node->data;
	free(((struct cil_call*)test_db->ast->root->cl_head->next->next->next->data)->macro_str);
	((struct cil_call*)test_db->ast->root->cl_head->next->next->next->data)->macro_str = NULL;

	((struct cil_call*)test_db->ast->root->cl_head->next->next->next->data)->macro->params->head->flavor = CIL_NETIFCON;

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_call1_unknowncall_neg(CuTest *tc) {
	char *line[] = {"(", "class", "foo", "(", "read", ")", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "class", "a", ")", ")",
				"(", "class", "b", "(", "read", ")", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "m", "(", "foo", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_call1_extraargs_neg(CuTest *tc) {
	char *line[] = {"(", "class", "foo", "(", "read", ")", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "class", "a", ")", ")",
				"(", "class", "b", "(", "read", ")", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "foo", "bar", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_call1_copy_neg(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "qaz", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "qaz", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_EEXIST, rc);
}

void test_cil_resolve_call1_missing_arg_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "l", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "h", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "macro", "mm", "(", "(", "level", "lvl_l", ")", "(", "level", "lvl_h", ")", ")",
				"(", "context", "foo", "(", "system_u", "role_r", "type_t", "lvl_l", "lvl_h", ")", ")", ")",
			"(", "call", "mm", "(", "l", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_call1_paramsflavor_neg(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "qaz", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tree_node *macro_node = NULL;

	struct cil_call *new_call = ((struct cil_call*)test_db->ast->root->cl_head->next->next->next->data);
	cil_resolve_name(test_db, test_db->ast->root->cl_head->next->next->next, new_call->macro_str, CIL_SYM_MACROS, NULL, &macro_node);
	new_call->macro = (struct cil_macro*)macro_node->data;
	struct cil_list_item *item = new_call->macro->params->head;
	item->flavor = CIL_CONTEXT;

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_call1_unknownflavor_neg(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "qaz", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tree_node *macro_node = NULL;

	struct cil_call *new_call = ((struct cil_call*)test_db->ast->root->cl_head->next->next->next->data);
	cil_resolve_name(test_db, test_db->ast->root->cl_head->next->next->next, new_call->macro_str, CIL_SYM_MACROS, NULL, &macro_node);
	new_call->macro = (struct cil_macro*)macro_node->data;
	struct cil_list_item *item = new_call->macro->params->head;
	((struct cil_param*)item->data)->flavor = CIL_CONTEXT;

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_call2_type(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "qaz", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call2_role(CuTest *tc) {
	char *line[] = {"(", "role", "role_r", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "role", "a", ")", ")",
				"(", "role", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "role_r", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call2_user(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "user", "a", ")", ")",
				"(", "user", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "user_u", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call2_sens(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "sens", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "sensitivity", "a", ")", ")",
				"(", "sensitivity", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "sens", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call2_cat(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "category", "a", ")", ")",
				"(", "category", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "c0", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call2_catset(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryset", "somecats", "(", "c0", "c1", "c2", ")", ")",
			"(", "macro", "mm", "(", "(", "categoryset",  "foo", ")", ")",
				"(", "level", "bar", "(", "s0", "foo", ")", ")", ")",
			"(", "call", "mm", "(", "somecats", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call2_catset_anon(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "macro", "mm", "(", "(", "categoryset",  "foo", ")", ")",
				"(", "level", "bar", "(", "s0", "foo", ")", ")", ")",
			"(", "call", "mm", "(", "(", "c0", "c1", "c2", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call2_permset(CuTest *tc) {
	char *line[] = {"(", "permissionset", "foo", "(", "read", "open", ")", ")",
			"(", "class", "dead", "(", "close", ")", ")",
			"(", "class", "bar", "(", "close", ")", ")",
			"(", "class", "baz", "(", "close", ")", ")",
			"(", "macro", "mm", "(", "(", "permissionset", "a", ")", ")", 
				"(", "allow", "dead", "bar", "baz", "a", ")", ")",
			"(", "call", "mm", "(", "foo", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call2_permset_anon(CuTest *tc) {
	char *line[] = {"(", "class", "dead", "(", "close", ")", ")",
			"(", "class", "bar", "(", "close", ")", ")",
			"(", "class", "baz", "(", "close", ")", ")",
			"(", "macro", "mm", "(", "(", "permissionset", "a", ")", ")", 
				"(", "allow", "dead", "bar", "baz", "a", ")", ")",
			"(", "call", "mm", "(", "(", "read", "open", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call2_class(CuTest *tc) {
	char *line[] = {"(", "class", "foo", "(", "read", ")", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "class", "a", ")", ")",
				"(", "class", "b", "(", "read", ")", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "foo", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call2_level(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "l", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "h", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "macro", "mm", "(", "(", "level", "lvl_l", ")", "(", "level", "lvl_h", ")", ")",
				"(", "context", "foo", "(", "system_u", "role_r", "type_t", "lvl_l", "lvl_h", ")", ")", ")",
			"(", "call", "mm", "(", "l", "h", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call2_level_anon(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "l", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "h", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "macro", "mm", "(", "(", "level", "lvl_l", ")", ")",
				"(", "context", "foo", "(", "system_u", "role_r", "type_t", "lvl_l", "h", ")", ")", ")",
			"(", "call", "mm", "(", "(", "s0", "(", "c0", ")", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call2_ipaddr(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "lvl_l", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "lvl_h", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "system_u", "role_r", "type_t", "lvl_l", "lvl_h", ")", ")",
			"(", "ipaddr", "netmask", "192.168.0.1", ")",
			"(", "ipaddr", "ip", "192.168.0.1", ")",
			"(", "macro", "mm", "(", "(", "ipaddr", "addr", ")", ")",
				"(", "nodecon", "addr", "netmask", "con", ")", ")",
			"(", "call", "mm", "(", "ip", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next->next, NULL);
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call2_ipaddr_anon(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "lvl_l", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "lvl_h", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "system_u", "role_r", "type_t", "lvl_l", "lvl_h", ")", ")",
			"(", "ipaddr", "netmask", "192.168.0.1", ")",
			"(", "macro", "mm", "(", "(", "ipaddr", "addr", ")", ")",
				"(", "nodecon", "addr", "netmask", "con", ")", ")",
			"(", "call", "mm", "(", "(", "192.168.1.1", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next, NULL);
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_call2_unknown_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "l", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "h", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "macro", "mm", "(", "(", "level", "lvl_l", ")", "(", "level", "lvl_h", ")", ")",
				"(", "context", "foo", "(", "system_u", "role_r", "type_t", "lvl_l", "lvl_h", ")", ")", ")",
			"(", "call", "mm", "(", "l", "h", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	((struct cil_args*)((struct cil_list_item *)((struct cil_call *)test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->data)->args->head)->data)->flavor = CIL_SYM_UNKNOWN;
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_call2_name_neg(CuTest *tc) {
	char *line[] = {"(", "class", "foo", "(", "read", ")", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "class", "a", ")", ")",
				"(", "class", "b", "(", "read", ")", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_name_call_args(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "qaz", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tree_node *test_node;
	cil_tree_node_init(&test_node);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_resolve_name_call_args((struct cil_call *)test_db->ast->root->cl_head->next->next->next->data, "a", CIL_SYM_TYPES, &test_node);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_name_call_args_multipleparams(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "l", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "h", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "macro", "mm", "(", "(", "level", "lvl_l", ")", "(", "level", "lvl_h", ")", ")",
				"(", "context", "foo", "(", "system_u", "role_r", "type_t", "lvl_l", "lvl_h", ")", ")", ")",
			"(", "call", "mm", "(", "l", "h", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	struct cil_tree_node *test_node;
	cil_tree_node_init(&test_node);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
	int rc = cil_resolve_name_call_args((struct cil_call *)test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->data, "lvl_h", CIL_SYM_LEVELS, &test_node);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_name_call_args_diffflavor(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "qaz", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tree_node *test_node;
	cil_tree_node_init(&test_node);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_resolve_name_call_args((struct cil_call *)test_db->ast->root->cl_head->next->next->next->data, "qaz", CIL_LEVEL, &test_node);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_name_call_args_callnull_neg(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "qaz", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tree_node *test_node;
	cil_tree_node_init(&test_node);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_resolve_name_call_args(NULL, "qaz", CIL_LEVEL, &test_node);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_name_call_args_namenull_neg(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "qaz", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tree_node *test_node;
	cil_tree_node_init(&test_node);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_resolve_name_call_args((struct cil_call *)test_db->ast->root->cl_head->next->next->next->data, NULL, CIL_LEVEL, &test_node);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_name_call_args_callargsnull_neg(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tree_node *test_node;
	cil_tree_node_init(&test_node);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_resolve_name_call_args((struct cil_call *)test_db->ast->root->cl_head->next->next->next->data, "qas", CIL_LEVEL, &test_node);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_name_call_args_name_neg(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "baz", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tree_node *test_node = NULL;
	//cil_tree_node_init(&test_node);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_resolve_name_call_args((struct cil_call *)test_db->ast->root->cl_head->next->next->next->data, "qas", CIL_TYPE, &test_node);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_expr_stack_bools(CuTest *tc) {
	char *line[] = {"(", "bool", "foo", "true", ")",
			"(", "bool", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "booleanif", "(", "&&", "foo", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	struct cil_booleanif *bif = (struct cil_booleanif*)test_db->ast->root->cl_head->next->next->next->data; 

	int rc = cil_resolve_expr_stack(test_db, bif->expr_stack,test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_expr_stack_tunables(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "&&", "foo", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	struct cil_tunableif *tif = (struct cil_tunableif*)test_db->ast->root->cl_head->next->next->next->data; 

	int rc = cil_resolve_expr_stack(test_db, tif->expr_stack,test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_expr_stack_type(CuTest *tc) {
        char *line[] = {"(", "class", "file", "(", "create", "relabelto", ")", ")",
			"(", "class", "dir", "(", "create", "relabelto", ")", ")",
			"(", "type", "t1", ")",
			"(", "type", "type_t", ")",
			"(", "mlsconstrain", "(", "file", "dir", ")", "(", "create", "relabelto", ")", "(", "==", "t1", "type_t", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	struct cil_constrain *cons = (struct cil_constrain*)test_db->ast->root->cl_head->next->next->next->next->data; 

	int rc = cil_resolve_expr_stack(test_db, cons->expr, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_expr_stack_role(CuTest *tc) {
        char *line[] = {"(", "class", "file", "(", "create", "relabelto", ")", ")",
			"(", "class", "dir", "(", "create", "relabelto", ")", ")",
			"(", "role", "r1", ")",
			"(", "role", "role_r", ")",
			"(", "mlsconstrain", "(", "file", "dir", ")", "(", "create", "relabelto", ")", "(", "==", "r1", "role_r", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	struct cil_constrain *cons = (struct cil_constrain*)test_db->ast->root->cl_head->next->next->next->next->data; 

	int rc = cil_resolve_expr_stack(test_db, cons->expr, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_expr_stack_user(CuTest *tc) {
        char *line[] = {"(", "class", "file", "(", "create", "relabelto", ")", ")",
			"(", "class", "dir", "(", "create", "relabelto", ")", ")",
			"(", "user", "u1", ")",
			"(", "user", "user_u", ")",
			"(", "mlsconstrain", "(", "file", "dir", ")", "(", "create", "relabelto", ")", "(", "==", "u1", "user_u", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	struct cil_constrain *cons = (struct cil_constrain*)test_db->ast->root->cl_head->next->next->next->next->data; 

	int rc = cil_resolve_expr_stack(test_db, cons->expr, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_expr_stack_neg(CuTest *tc) {
	char *line[] = {"(", "bool", "foo", "true", ")",
			"(", "bool", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "booleanif", "(", "&&", "beef", "baf", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	struct cil_booleanif *bif = (struct cil_booleanif*)test_db->ast->root->cl_head->next->next->next->data; 

	int rc = cil_resolve_expr_stack(test_db, bif->expr_stack,test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_expr_stack_emptystr_neg(CuTest *tc) {
	char *line[] = {"(", "bool", "foo", "true", ")",
			"(", "bool", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "booleanif", "(", "&&", "foo", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	struct cil_booleanif *bif = (struct cil_booleanif*)test_db->ast->root->cl_head->next->next->next->data;
	((struct cil_conditional*)bif->expr_stack->data)->str = NULL;

	int rc = cil_resolve_expr_stack(test_db, bif->expr_stack,test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_boolif(CuTest *tc) {
	char *line[] = {"(", "bool", "foo", "true", ")",
			"(", "bool", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "booleanif", "(", "&&", "foo", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	int rc = cil_resolve_boolif(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_boolif_neg(CuTest *tc) {
	char *line[] = {"(", "bool", "foo", "true", ")",
			"(", "bool", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "booleanif", "(", "&&", "dne", "N/A", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	int rc = cil_resolve_boolif(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_evaluate_expr_stack_and(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "&&", "foo", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint16_t result = CIL_FALSE;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tunableif *tif = (struct cil_tunableif*)test_db->ast->root->cl_head->next->next->next->data;

	cil_resolve_expr_stack(test_db, tif->expr_stack, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_evaluate_expr_stack(tif->expr_stack, &result);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_evaluate_expr_stack_not(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "!", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint16_t result = CIL_FALSE;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tunableif *tif = (struct cil_tunableif*)test_db->ast->root->cl_head->next->next->next->data;

	cil_resolve_expr_stack(test_db, tif->expr_stack, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_evaluate_expr_stack(tif->expr_stack, &result);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_evaluate_expr_stack_or(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "||", "foo", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint16_t result = CIL_FALSE;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tunableif *tif = (struct cil_tunableif*)test_db->ast->root->cl_head->next->next->next->data;

	cil_resolve_expr_stack(test_db, tif->expr_stack, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_evaluate_expr_stack(tif->expr_stack, &result);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_evaluate_expr_stack_xor(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "^", "foo", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint16_t result = CIL_FALSE;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tunableif *tif = (struct cil_tunableif*)test_db->ast->root->cl_head->next->next->next->data;

	cil_resolve_expr_stack(test_db, tif->expr_stack, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_evaluate_expr_stack(tif->expr_stack, &result);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_evaluate_expr_stack_eq(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "==", "foo", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint16_t result = CIL_FALSE;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tunableif *tif = (struct cil_tunableif*)test_db->ast->root->cl_head->next->next->next->data;

	cil_resolve_expr_stack(test_db, tif->expr_stack, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_evaluate_expr_stack(tif->expr_stack, &result);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_evaluate_expr_stack_neq(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "!=", "foo", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint16_t result = CIL_FALSE;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tunableif *tif = (struct cil_tunableif*)test_db->ast->root->cl_head->next->next->next->data;

	cil_resolve_expr_stack(test_db, tif->expr_stack, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_evaluate_expr_stack(tif->expr_stack, &result);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_evaluate_expr_stack_oper1(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "false", ")",
			"(", "tunable", "baz", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "&&", "(", "||", "foo", "bar", ")", "baz", ")",
			"(", "allow", "foo", "bar", "jaz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint16_t result = CIL_FALSE;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tunableif *tif = (struct cil_tunableif*)test_db->ast->root->cl_head->next->next->next->next->data;

	cil_resolve_expr_stack(test_db, tif->expr_stack, test_db->ast->root->cl_head->next->next->next->next, NULL);
	int rc = cil_evaluate_expr_stack(tif->expr_stack, &result);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_evaluate_expr_stack_oper2(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "false", ")",
			"(", "tunable", "baz", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "&&", "baz", "(", "||", "foo", "bar", ")", ")",
			"(", "allow", "foo", "bar", "jaz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint16_t result = CIL_FALSE;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tunableif *tif = (struct cil_tunableif*)test_db->ast->root->cl_head->next->next->next->next->data;

	cil_resolve_expr_stack(test_db, tif->expr_stack, test_db->ast->root->cl_head->next->next->next->next, NULL);
	int rc = cil_evaluate_expr_stack(tif->expr_stack, &result);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_evaluate_expr_stack_neg(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "!=", "foo", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint16_t result = CIL_FALSE;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tree_node *test_node;
	cil_tree_node_init(&test_node);

	struct cil_conditional *new_cond;
	cil_conditional_init(&new_cond);
	new_cond->flavor = CIL_COND;
	char *baz = "baz";
	new_cond->str = baz;
	new_cond->flavor = CIL_TUNABLE;

	test_node->data = new_cond;	
	test_node->cl_head = ((struct cil_tunableif*)test_db->ast->root->cl_head->next->next->next->data)->expr_stack;
	((struct cil_tunableif*)test_db->ast->root->cl_head->next->next->next->data)->expr_stack->parent = test_node;
	struct cil_tunableif *tif = (struct cil_tunableif*)test_db->ast->root->cl_head->next->next->next->data;
	cil_resolve_expr_stack(test_db, tif->expr_stack, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = cil_evaluate_expr_stack(tif->expr_stack, &result);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_tunif_false(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "&&", "foo", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	int rc = cil_resolve_tunif(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_tunif_true(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "true", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "&&", "foo", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	int rc = cil_resolve_tunif(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_tunif_resolveexpr_neg(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "&&", "dne", "N/A", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	int rc = cil_resolve_tunif(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_tunif_evaluateexpr_neg(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "&&", "foo", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	struct cil_tree_node *test_node;
	cil_tree_node_init(&test_node);

	struct cil_conditional *new_cond;
	cil_conditional_init(&new_cond);
	new_cond->flavor = CIL_COND;
	char *baz = "baz";
	new_cond->str = baz;
	new_cond->flavor = CIL_TUNABLE;

	test_node->data = new_cond;	
	test_node->cl_head = ((struct cil_tunableif*)test_db->ast->root->cl_head->next->next->next->data)->expr_stack;
	((struct cil_tunableif*)test_db->ast->root->cl_head->next->next->next->data)->expr_stack->parent = test_node;

	int rc = cil_resolve_tunif(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_userbounds(CuTest *tc) {
	char *line[] = {"(", "user", "user1", ")", 
			"(", "user", "user2", ")", 
			"(", "userbounds", "user1", "user2", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_userbounds(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_userbounds_exists_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user1", ")", 
			"(", "user", "user2", ")", 
			"(", "userbounds", "user1", "user2", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_userbounds(test_db, test_db->ast->root->cl_head->next->next, NULL);
	int rc = cil_resolve_userbounds(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_userbounds_user1_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user1", ")", 
			"(", "user", "user2", ")", 
			"(", "userbounds", "user_DNE", "user2", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_userbounds(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_userbounds_user2_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user1", ")", 
			"(", "user", "user2", ")", 
			"(", "userbounds", "user1", "user_DNE", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_userbounds(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_roletype(CuTest *tc) {
	char *line[] = {"(", "role",  "admin_r", ")",
			"(", "type", "admin_t", ")",
			"(", "roletype", "admin_r", "admin_t", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roletype(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_roletype_type_neg(CuTest *tc) {
	char *line[] = {"(", "role",  "admin_r", ")",
			"(", "roletype", "admin_r", "admin_t", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roletype(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_roletype_role_neg(CuTest *tc) {
	char *line[] = {"(", "type", "admin_t", ")",
			"(", "roletype", "admin_r", "admin_t", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roletype(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_userrole(CuTest *tc) {
	char *line[] = {"(", "role",  "staff_r", ")",
			"(", "user", "staff_u", ")",
			"(", "userrole", "staff_u", "staff_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_userrole(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_userrole_user_neg(CuTest *tc) {
	char *line[] = {"(", "role",  "staff_r", ")",
			"(", "userrole", "staff_u", "staff_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_userrole(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_userrole_role_neg(CuTest *tc) {
	char *line[] = {"(", "user", "staff_u", ")",
			"(", "userrole", "staff_u", "staff_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_userrole(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_disable_children_helper_optional_enabled(CuTest *tc) {
	char *line[] = {"(", "optional", "opt", "(", "allow", "foo", "bar", "baz", "file", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_optional_disabled(CuTest *tc) {
	char *line[] = {"(", "optional", "opt", "(", "allow", "foo", "bar", "baz", "file", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	uint32_t finished = 0;

	((struct cil_optional *)test_db->ast->root->cl_head->data)->datum.state = CIL_STATE_DISABLED;
	
	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_block(CuTest *tc) {
	char *line[] = {"(", "block", "a", "(", "type", "log", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_user(CuTest *tc) {
	char *line[] = {"(", "user", "staff_u", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_role(CuTest *tc) {
	char *line[] = {"(", "role", "role_r", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_type(CuTest *tc) {
	char *line[] = {"(", "type", "type_t", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_typealias(CuTest *tc) {
	char *line[] = {"(", "typealias", ".test.type", "type_t", ")", "(", "type", "test", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_common(CuTest *tc) {
	char *line[] = {"(", "common", "foo", "(", "read", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_class(CuTest *tc) {
	char *line[] = {"(", "class", "foo", "(", "read", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_bool(CuTest *tc) {
	char *line[] = {"(", "bool", "foo", "true", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_sens(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_cat(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_catset(CuTest *tc) {
	char *line[] = {"(", "categoryset", "somecats", "(", "c0", "c1", "c2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_sid(CuTest *tc) {
	char *line[] = {"(", "sid", "foo", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_macro(CuTest *tc) {
	char *line[] = {"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_context(CuTest *tc) {
	char *line[] = {"(", "context", "con",
                        "(", "system_u", "object_r", "netif_t", "low", "high", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_level(CuTest *tc) {
	char *line[] = {"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_policycap(CuTest *tc) {
	char *line[] = {"(", "policycap", "foo", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_perm(CuTest *tc) {
	char *line[] = {"(", "class", "foo", "(", "read", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_catalias(CuTest *tc) {
	char *line[] = {"(", "categoryalias", "c0", "red", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_sensalias(CuTest *tc) {
	char *line[] = {"(", "sensitivityalias", "s0", "red", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_tunable(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "false", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_disable_children_helper_unknown(CuTest *tc) {
	char *line[] = {"(", "mlsconstrain", "(", "file", "dir", ")", "(", "create", "relabelto", ")", "(", "eq", "l2", "h2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	uint32_t finished = 0;

	int rc = __cil_disable_children_helper(test_db->ast->root->cl_head, &finished, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}


/*
	__cil_resolve_ast_node_helper test cases
*/

void test_cil_resolve_ast_node_helper_call1(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "qaz", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 2;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_call1_neg(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "m", "(", "qaz", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 2;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_call2(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "qaz", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 3;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_call2_neg(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "foo", "extra", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 3;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_ast_node_helper_boolif(CuTest *tc) {
	char *line[] = {"(", "bool", "foo", "true", ")",
			"(", "bool", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "booleanif", "(", "&&", "foo", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 4;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_boolif_neg(CuTest *tc) {
	char *line[] = {"(", "bool", "foo", "true", ")",
			"(", "bool", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "booleanif", "(", "&&", "dne", "N/A", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 4;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_tunif(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "&&", "foo", "bar", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 1;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_tunif_neg(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")",
			"(", "tunable", "bar", "false", ")",
			"(", "class", "baz", "(", "read", ")", ")",
			"(", "tunableif", "(", "&&", "dne", "N/A", ")",
			"(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 1;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_catorder(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")", 
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryorder", "(", "c0", "c1", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 4;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);
	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_catorder_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")", 
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryorder", "(", "c8", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 4;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_dominance(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")", 
			"(", "sensitivity", "s1", ")",
			"(", "sensitivity", "s2", ")",
			"(", "dominance", "(", "s0", "s1", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 4;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_dominance_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")", 
			"(", "sensitivity", "s1", ")",
			"(", "sensitivity", "s2", ")",
			"(", "dominance", "(", "s0", "s6", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 4;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_roleallow(CuTest *tc) {
	char *line[] = {"(", "role", "foo", ")", \
			"(", "role", "bar", ")", \
			"(", "roleallow", "foo", "bar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);	
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_roleallow_neg(CuTest *tc) {
	char *line[] = {"(", "role", "foo", ")", \
			"(", "roleallow", "foo", "bar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_roledominance(CuTest *tc) {
	char *line[] = {"(", "role", "foo", ")", \
			"(", "role", "bar", ")", \
			"(", "roledominance", "foo", "bar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);	
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_roledominance_neg(CuTest *tc) {
	char *line[] = {"(", "role", "foo", ")", \
			"(", "roledominance", "foo", "bar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_sensalias(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "sensitivityalias", "s0", "alias", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_sensalias_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivityalias", "s0", "alias", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_catalias(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")", 
			"(", "categoryalias", "c0", "red", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass =7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_catalias_neg(CuTest *tc) {
	char *line[] = {"(", "categoryalias", "c0", "red", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);
	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_catset(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "category", "c2", ")",
			"(", "categoryset", "somecats", "(", "c0", "c1", "c2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 5;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_catset_catlist_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "categoryset", "somecats", "(", "c0", "c1", "c2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 5;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_level(CuTest *tc) {
        char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c1", ")",
			"(", "sensitivitycategory", "s0", "(", "c1", ")", ")",
			"(", "level", "l2", "(", "s0", "(", "c1", ")", ")", ")",  NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 4;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_tree_walk(test_db->ast->root,  __cil_resolve_ast_node_helper, NULL, NULL, extra_args);
	__cil_verify_order(test_db->catorder, test_db->ast->root, CIL_CAT);

	__cil_verify_order(test_db->dominance, test_db->ast->root, CIL_SENS);

	pass = 5;
	cil_tree_walk(test_db->ast->root, __cil_resolve_ast_node_helper, NULL, NULL, extra_args);

	pass = 6;
	cil_tree_walk(test_db->ast->root, __cil_resolve_ast_node_helper, NULL, NULL, extra_args);

	pass = 7;
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, finished, 0);
}

void test_cil_resolve_ast_node_helper_level_neg(CuTest *tc) {
        char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c1", ")",
			"(", "sensitivitycategory", "s0", "(", "c1", ")", ")",
			"(", "level", "l2", "(", "s8", "(", "c1", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 4;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_tree_walk(test_db->ast->root,  __cil_resolve_ast_node_helper, NULL, NULL, extra_args);
	__cil_verify_order(test_db->catorder, test_db->ast->root, CIL_CAT);

	__cil_verify_order(test_db->dominance, test_db->ast->root, CIL_SENS);

	pass = 5;
	cil_tree_walk(test_db->ast->root, __cil_resolve_ast_node_helper, NULL, NULL, extra_args);

	pass = 7;

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, finished, 0);
}

void test_cil_resolve_ast_node_helper_constrain(CuTest *tc) {
        char *line[] = {"(", "class", "file", "(", "create", "relabelto", ")", ")",
			"(", "class", "dir", "(", "create", "relabelto", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c1", ")",
			"(", "role", "r1", ")",
			"(", "role", "r2", ")",
			"(", "constrain", "(", "file", "dir", ")", "(", "create", "relabelto", ")", "(", "eq", "r1", "r2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_constrain_neg(CuTest *tc) {
        char *line[] = {"(", "class", "file", "(", "create", ")", ")",
			"(", "class", "dir", "(", "create", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c1", ")",
			"(", "role", "r1", ")",
			"(", "role", "r2", ")",
			"(", "constrain", "(", "file", "dir", ")", "(", "create", "relabelto", ")", "(", "eq", "r1", "r2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_mlsconstrain(CuTest *tc) {
        char *line[] = {"(", "class", "file", "(", "create", "relabelto", ")", ")",
			"(", "class", "dir", "(", "create", "relabelto", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c1", ")",
			"(", "level", "l2", "(", "s0", "(", "c1", ")", ")", ")",
			"(", "level", "h2", "(", "s0", "(", "c1", ")", ")", ")",
			"(", "mlsconstrain", "(", "file", "dir", ")", "(", "create", "relabelto", ")", "(", "eq", "l2", "h2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_mlsconstrain_neg(CuTest *tc) {
        char *line[] = {"(", "class", "file", "(", "read", ")", ")",
			"(", "class", "dir", "(", "read", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c1", ")",
			"(", "level", "l2", "(", "s0", "(", "c1", ")", ")", ")",
			"(", "level", "h2", "(", "s0", "(", "c1", ")", ")", ")",
			"(", "mlsconstrain", "(", "file", "dir", ")", "(", "create", "relabelto", ")", "(", "eq", "l2", "h2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_context(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con",
                        "(", "system_u", "object_r", "netif_t", "low", "high", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 4;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_tree_walk(test_db->ast->root,  __cil_resolve_ast_node_helper, NULL, NULL, extra_args);
	__cil_verify_order(test_db->catorder, test_db->ast->root, CIL_CAT);

	__cil_verify_order(test_db->dominance, test_db->ast->root, CIL_SENS);

	pass = 5;
	cil_tree_walk(test_db->ast->root, __cil_resolve_ast_node_helper, NULL, NULL, extra_args);

	pass = 7;

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, finished, 0);
}

void test_cil_resolve_ast_node_helper_context_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con",
                        "(", "system_u", "object_r", "netif_t", "llow", "high", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 4;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_tree_walk(test_db->ast->root,  __cil_resolve_ast_node_helper, NULL, NULL, extra_args);
	__cil_verify_order(test_db->catorder, test_db->ast->root, CIL_CAT);

	__cil_verify_order(test_db->dominance, test_db->ast->root, CIL_SENS);

	pass = 5;
	cil_tree_walk(test_db->ast->root, __cil_resolve_ast_node_helper, NULL, NULL, extra_args);

	pass = 7;

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, finished, 0);
}

void test_cil_resolve_ast_node_helper_senscat(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
                        "(", "sensitivity", "s1", ")",
                        "(", "dominance", "(", "s0", "s1", ")", ")",
                        "(", "category", "c0", ")",
                        "(", "category", "c255", ")",
                        "(", "categoryorder", "(", "c0", "c255", ")", ")",
                        "(", "sensitivitycategory", "s1", "(", "c0", "c255", ")", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 6;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_senscat_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
                        "(", "sensitivity", "s1", ")",
                        "(", "dominance", "(", "s0", "s1", ")", ")",
                        "(", "category", "c0", ")",
                        "(", "category", "c255", ")",
                        "(", "categoryorder", "(", "c0", "c255", ")", ")",
                        "(", "sensitivitycategory", "s5", "foo", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 6;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_roletrans(CuTest *tc) {
	char *line[] = {"(", "role", "foo_r", ")",
			"(", "type", "bar_t", ")",
			"(", "role", "foobar_r", ")", 
			"(", "class", "process", "(", "transition", ")", ")",
			"(", "roletransition", "foo_r", "bar_t", "process", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_roletrans_srcdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "bar_t", ")",
			"(", "role", "foobar_r", ")", 
			"(", "class", "process", "(", "transition", ")", ")",
			"(", "roletransition", "foo_r", "bar_t", "process", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_roletrans_tgtdecl_neg(CuTest *tc) {
	char *line[] = {"(", "role", "foo_r", ")",
			"(", "role", "foobar_r", ")", 
			"(", "class", "process", "(", "transition", ")", ")",
			"(", "roletransition", "foo_r", "bar_t", "process", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_roletrans_resultdecl_neg(CuTest *tc) {
	char *line[] = {"(", "role", "foo_r", ")",
			"(", "type", "bar_t", ")",
			"(", "class", "process", "(", "transition", ")", ")",
			"(", "roletransition", "foo_r", "bar_t", "process", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_typeattr(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "attribute", "bar", ")",
			"(", "typeattribute", "foo", "bar", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);	
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_typeattr_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "typeattribute", "foo", "bar", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_attrtypes(CuTest *tc) {
	char *line[] = {"(", "attribute", "attrs", ")",
			"(", "type", "type_t", ")",
			"(", "type", "type_tt", ")",
			"(", "attributetypes", "attrs", "(", "type_t", "type_tt", ")", ")", NULL}; 

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_attrtypes_neg(CuTest *tc) {
	char *line[] = {"(", "attribute", "attrs", ")",
			"(", "type", "type_t", ")",
			"(", "attributetypes", "attrs", "(", "type_t", "-t_t", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_typealias(CuTest *tc) {
	char *line[] = {"(", "block", "foo", 
				"(", "typealias", ".foo.test", "type_t", ")", 
				"(", "type", "test", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->cl_head, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_typealias_notype_neg(CuTest *tc) {
	char *line[] = {"(", "block", "bar", 
				"(", "typealias", ".bar.test", "type_t", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->cl_head, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_typebounds(CuTest *tc) {
	char *line[] = {"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "typebounds", "type_a", "type_b", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_typebounds_neg(CuTest *tc) {
	char *line[] = {"(", "type", "type_b", ")",
			"(", "typebounds", "type_a", "type_b", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_typepermissive(CuTest *tc) {
	char *line[] = {"(", "type", "type_a", ")",
			"(", "typepermissive", "type_a", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_typepermissive_neg(CuTest *tc) {
	char *line[] = {"(", "type", "type_b", ")",
			"(", "typepermissive", "type_a", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_rangetransition(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "rangetransition", "type_a", "type_b", "class_", "low", "high", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_rangetransition_neg(CuTest *tc) {
	char *line[] = {"(", "class", "class_", "(", "read", ")", ")",
			"(", "type", "type_a", ")",
			"(", "type", "type_b", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "rangetransition", "type_DNE", "type_b", "class_", "low", "high", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_filetransition(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "type", "foobar", ")",
			"(", "filetransition", "foo", "bar", "file", "foobar", "str", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_filetransition_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "type", "foobar", ")",
			"(", "filetransition", "foo", "bar", "file", "foobarrr", "str", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_avrule(CuTest *tc) {
	char *line[] = {"(", "class", "bar", "(", "read", "write", "open", ")", ")", 
	                "(", "type", "test", ")", 
			"(", "type", "foo", ")", 
	                "(", "allow", "test", "foo", "bar", "(", "read", "write", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_avrule_src_nores_neg(CuTest *tc) {
	char *line[] = {"(", "allow", "test", "foo", "bar", "(", "read", "write", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_avrule_tgt_nores_neg(CuTest *tc) {
	char *line[] = {"(", "type", "test", ")", 
			"(", "allow", "test", "foo", "bar", "(", "read", "write", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_avrule_class_nores_neg(CuTest *tc) {
	char *line[] = {"(", "type", "test", ")", 
			"(", "type", "foo", ")", 
			"(", "allow", "test", "foo", "bar", "(", "read", "write", ")", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_avrule_datum_null_neg(CuTest *tc) {
	char *line[] = {"(", "class", "bar", "(", "read", "write", "open", ")", ")", \
	                "(", "type", "test", ")", "(", "type", "foo", ")", \
	                "(", "allow", "test", "foo", "bar", "(","fake", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_type_rule_transition(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "type", "foobar", ")",
			"(", "typetransition", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_type_rule_transition_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "type", "foobar", ")",
			"(", "typetransition", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_type_rule_change(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "type", "foobar", ")",
			"(", "typechange", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_type_rule_change_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "type", "foobar", ")",
			"(", "typechange", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_type_rule_member(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "type", "foobar", ")",
			"(", "typemember", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_type_rule_member_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "class", "file", "(", "write", ")", ")",
			"(", "type", "foobar", ")",
			"(", "typemember", "foo", "bar", "file", "foobar", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_userbounds(CuTest *tc) {
	char *line[] = {"(", "user", "user1", ")",
			"(", "user", "user2", ")",
			"(", "userbounds", "user1", "user2", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, 0, finished);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_ast_node_helper_userbounds_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user1", ")",
			"(", "userbounds", "user1", "user2", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
	CuAssertIntEquals(tc, 0, finished);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_ast_node_helper_roletype(CuTest *tc) {
	char *line[] = {"(", "role",  "admin_r", ")",
			"(", "type", "admin_t", ")",
			"(", "roletype", "admin_r", "admin_t", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, 0, finished);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_ast_node_helper_roletype_role_neg(CuTest *tc) {
	char *line[] = {"(", "type", "admin_t", ")",
			"(", "roletype", "admin_r", "admin_t", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
	CuAssertIntEquals(tc, 0, finished);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_ast_node_helper_roletype_type_neg(CuTest *tc) {
	char *line[] = {"(", "role", "admin_r", ")",
			"(", "roletype", "admin_r", "admin_t", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
	CuAssertIntEquals(tc, 0, finished);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_ast_node_helper_userrole(CuTest *tc) {
	char *line[] = {"(", "role",  "staff_r", ")",
			"(", "user", "staff_u", ")",
			"(", "userrole", "staff_u", "staff_r", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, 0, finished);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_ast_node_helper_userrole_user_neg(CuTest *tc) {
	char *line[] = {"(", "role",  "staff_r", ")",
			"(", "userrole", "staff_u", "staff_r", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
	CuAssertIntEquals(tc, 0, finished);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_ast_node_helper_userrole_role_neg(CuTest *tc) {
	char *line[] = {"(", "user",  "staff_u", ")",
			"(", "userrole", "staff_u", "staff_r", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
	CuAssertIntEquals(tc, 0, finished);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_ast_node_helper_filecon(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "filecon", "root", "path", "file", "con", ")", NULL};
        
	struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_OK, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_filecon_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "filecon", "root", "path", "file", "foo", ")", NULL};
 
        struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_portcon(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "portcon", "type", "25", "con", ")", NULL};
        
	struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_OK, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_portcon_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "portcon", "type", "25", "foo", ")", NULL};
 
        struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_genfscon(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "genfscon", "type", "path", "con", ")", NULL};
        
	struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_OK, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_genfscon_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "genfscon", "type", "path", "foo", ")", NULL};
 
        struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_nodecon(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "ipaddr", "ip", "192.168.1.1", ")",
			"(", "nodecon", "ip", "ip", "con", ")", NULL};
        
	struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_OK, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_nodecon_ipaddr_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "ipaddr", "ip", "192.168.1.1", ")",
			"(", "ipaddr", "netmask", "192.168.1.1", ")",
			"(", "nodecon", "ipp", "netmask", "foo", ")", NULL};
 
        struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_nodecon_netmask_neg(CuTest *tc) {
	char *line[] = {"(", "user", "user_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "context", "con", "(", "user_u", "role_r", "type_t", "low", "high", ")", ")",
			"(", "ipaddr", "ip", "192.168.1.1", ")",
			"(", "ipaddr", "netmask", "192.168.1.1", ")",
			"(", "nodecon", "ip", "nnetmask", "foo", ")", NULL};
 
        struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_netifcon(CuTest *tc) {
	char *line[] = {"(", "context", "if_default", "(", "system_u", "object_r", "etc_t", "low", "high", ")", ")",
			"(", "context", "packet_default", "(", "system_u", "object_r", "etc_t", "low", "high", ")", ")", 
			"(", "netifcon", "eth0", "if_default", "packet_default", ")", NULL};
        
	struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_OK, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_netifcon_neg(CuTest *tc) {
	char *line[] = {"(", "context", "if_default", "(", "system_u", "object_r", "etc_t", "low", "high", ")", ")",
			"(", "netifcon", "eth0", "if_default", "packet_default", ")", NULL};
 
        struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_fsuse(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con", "(", "system_u", "object_r", "netif_t", "low", "high", ")", ")",
			"(", "fsuse", "xattr", "ext3", "con", ")", NULL};
        
	struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_OK, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_fsuse_neg(CuTest *tc) {
	char *line[] = {"(", "context", "if_default", "(", "system_u", "object_r", "etc_t", "low", "high", ")", ")",
			"(", "netifcon", "eth0", "if_default", "packet_default", ")", NULL};
 
        struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_sidcontext(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "categoryorder", "(", "c0", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "type", "blah_t", ")",
			"(", "role", "blah_r", ")",
			"(", "user", "blah_u", ")",
			"(", "level", "low", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "level", "high", "(", "s0", "(", "c0", ")", ")", ")",
			"(", "sid", "test", "(", "blah_u", "blah_r", "blah_t", "low", "high", ")", ")", NULL};
        
	struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_OK, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_sidcontext_neg(CuTest *tc) {
       	char *line[] = {"(", "category", "c0", ")",
                        "(", "categoryorder", "(", "c0", ")", ")",
                        "(", "sensitivity", "s0", ")",
                        "(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
                        "(", "type", "blah_t", ")",
                        "(", "role", "blah_r", ")",
                        "(", "user", "blah_u", ")",
                        "(", "sidcontext", "test", "(", "", "blah_r", "blah_t", "(", "s0", "(", "c0", ")", ")", "(", "s0", "(", "c0", ")", ")", ")", ")", NULL};
 
        struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_classcommon(CuTest *tc) {
	char *line[] = {"(", "class", "file", "(", "read", ")", ")",
			"(", "common", "file", "(", "write", ")", ")",	
			"(", "classcommon", "file", "file", ")", NULL};
        
	struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 6;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_OK, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_classcommon_neg(CuTest *tc) {
	char *line[] = {"(", "class", "file", "(", "read", ")", ")",
			"(", "classcommon", "file", "file", ")", NULL};
        
	struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	uint32_t pass = 6;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
        CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_rolebounds(CuTest *tc) {
	char *line[] = {"(", "role", "role1", ")",
			"(", "role", "role2", ")",
			"(", "rolebounds", "role1", "role2", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, 0, finished);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_ast_node_helper_rolebounds_neg(CuTest *tc) {
	char *line[] = {"(", "role", "role1", ")",
			"(", "rolebounds", "role1", "role2", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
	CuAssertIntEquals(tc, 0, finished);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}


void test_cil_resolve_ast_node_helper_callstack(CuTest *tc) {
	char *line[] = {"(", "call", "mm", "(", "foo", ")", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	struct cil_tree_node *test_ast_node_call;
	cil_tree_node_init(&test_ast_node_call);
	test_ast_node_call->flavor = CIL_CALL;

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	__cil_resolve_ast_node_helper(test_db->ast->root->cl_head, &finished, extra_args);
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_ast_node_helper_call(CuTest *tc) {
	char *line[] = {"(", "call", "mm", "(", "foo", ")", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	struct cil_tree_node *test_ast_node_call;
	cil_tree_node_init(&test_ast_node_call);
	test_ast_node_call->flavor = CIL_CALL;

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_ast_node_helper_optional(CuTest *tc) {
	char *line[] = {"(", "optional", "opt", "(", "allow", "foo", "bar", "file", "(", "read", ")", ")", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	struct cil_tree_node *test_ast_node_opt;
	cil_tree_node_init(&test_ast_node_opt);
	test_ast_node_opt->flavor = CIL_OPTIONAL;

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	// set optional to disabled
	((struct cil_symtab_datum *)test_db->ast->root->cl_head->data)->state = CIL_STATE_DISABLED;

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_ast_node_helper_macro(CuTest *tc) {
	char *line[] = {"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_ast_node_helper_optstack(CuTest *tc) {
	char *line[] = {"(", "class", "baz", "(", "read", ")", ")",
			"(", "type", "foo", ")",
			"(", "type", "bar", ")",
			"(", "optional", "opt", "(", "allow", "foo", "bar", "baz", "(", "read", ")", ")", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	struct cil_tree_node *test_ast_node_opt;
	cil_tree_node_init(&test_ast_node_opt);
	test_ast_node_opt->flavor = CIL_OPTIONAL;

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	__cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_ast_node_helper_optstack_tunable_neg(CuTest *tc) {
	char *line[] = {"(", "tunable", "foo", "true", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_tree_node *test_ast_node_opt;
	cil_tree_node_init(&test_ast_node_opt);
	test_ast_node_opt->flavor = CIL_OPTIONAL;

	uint32_t pass = 1;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, test_ast_node_opt);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_ast_node_helper_optstack_macro_neg(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "qaz", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_tree_node *test_ast_node_opt;
	cil_tree_node_init(&test_ast_node_opt);
	test_ast_node_opt->flavor = CIL_OPTIONAL;

	uint32_t pass = 3;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, test_ast_node_opt);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next, NULL);
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_nodenull_neg(CuTest *tc) {
	char *line[] = {"(", "role",  "staff_r", ")",
			"(", "user", "staff_u", ")",
			"(", "userrole", "staff_u", "staff_r", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	uint32_t pass = 1;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	int rc = __cil_resolve_ast_node_helper(NULL, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_ast_node_helper_extraargsnull_neg(CuTest *tc) {
	char *line[] = {"(", "role",  "staff_r", ")",
			"(", "user", "staff_u", ")",
			"(", "userrole", "staff_u", "staff_r", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, NULL);
	CuAssertIntEquals(tc, 0, finished);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_ast_node_helper_dbflavor_neg(CuTest *tc) {
	char *line[] = {"(", "role",  "staff_r", ")",
			"(", "user", "staff_u", ")",
			"(", "userrole", "staff_u", "staff_r", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_ast_node_helper_pass_neg(CuTest *tc) {
	char *line[] = {"(", "role",  "staff_r", ")",
			"(", "user", "staff_u", ")",
			"(", "userrole", "staff_u", "staff_r", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_tree_node *test_ast_node;
	cil_tree_node_init(&test_ast_node);

	uint32_t finished = 0;

	uint32_t pass = 7;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, NULL);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, extra_args);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_ast_node_helper_optfailedtoresolve(CuTest *tc) {
	char *line[] = {"(", "class", "file", "(", "read", ")", ")",
			"(", "classcommon", "file", "file", ")", NULL};
        
	struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

	struct cil_optional *opt;
	cil_optional_init(&opt);

	struct cil_tree_node *test_ast_node_opt;
	cil_tree_node_init(&test_ast_node_opt);
	test_ast_node_opt->flavor = CIL_OPTIONAL;
	test_ast_node_opt->data = opt;

    uint32_t pass = 6;
	uint32_t changed = 0;
	struct cil_args_resolve *extra_args = gen_resolve_args(test_db, &pass, &changed, NULL, test_ast_node_opt);

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, extra_args);
        CuAssertIntEquals(tc, SEPOL_OK, rc);
        CuAssertIntEquals(tc, 0, finished);
}

