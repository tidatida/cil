#include <sepol/policydb/policydb.h>

#include "CuTest.h"
#include "CilTest.h"

#include "../../src/cil_build_ast.h"
#include "../../src/cil_resolve_ast.h"

int __cil_verify_order(struct cil_list *order, struct cil_tree_node *current, uint32_t flavor);
int __cil_resolve_ast_node_helper(struct cil_tree_node *, uint32_t *, struct cil_list *);

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

	uint32_t test_flavor = 0;

	int rc = cil_resolve_name(test_db, test_curr, test_alias->type_str, CIL_SYM_TYPES, test_flavor, NULL, &type_node);
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

	uint32_t test_flavor = 0;

	int rc = cil_resolve_name(test_db, test_curr, test_alias->type_str, CIL_SYM_TYPES, test_flavor, NULL, &type_node);
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
	
	struct cil_list *test_cat_list;
	cil_list_init(&test_cat_list);

	struct cil_catset *test_catset = (struct cil_catset*)test_db->ast->root->cl_head->next->next->next->next->data;

	int rc = cil_resolve_cat_list(test_db, test_db->ast->root->cl_head->next->next->next->next, test_catset->cat_list_str, test_cat_list, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
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

void test_cil_resolve_senscat_sublist_neg(CuTest *tc) {
      char *line[] = {"(", "sensitivity", "s0", ")",
                        "(", "sensitivity", "s1", ")",
                        "(", "dominance", "(", "s0", "s1", ")", ")",
                        "(", "category", "c0", ")",
                        "(", "category", "c255", ")",
                        "(", "categoryorder", "(", "c0", "c255", ")", ")",
                        "(", "sensitivitycategory", "s1", "(", "c0", "(", "c255", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
        other->head->flavor = CIL_DB;
    	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 3;
    	other->head->next->data = &pass;
    	cil_list_item_init(&other->head->next->next);
    	other->head->next->next->data = NULL;
    	cil_list_item_init(&other->head->next->next->next);
    	other->head->next->next->next->data = NULL;

	cil_tree_walk(test_db->ast->root, __cil_resolve_ast_node_helper, NULL, NULL, other);
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
        other->head->flavor = CIL_DB;
    	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 3;
    	other->head->next->data = &pass;
    	cil_list_item_init(&other->head->next->next);
    	other->head->next->next->data = NULL;
    	cil_list_item_init(&other->head->next->next->next);
    	other->head->next->next->next->data = NULL;

	cil_tree_walk(test_db->ast->root, __cil_resolve_ast_node_helper, NULL, NULL, other);
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, SEPOL_OK, rc2);
}

void test_cil_resolve_level_sens_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "categoryorder", "(", "c0", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "type", "blah_t", ")",
			"(", "role", "blah_r", ")",
			"(", "user", "blah_u", ")",
			"(", "level", "low", "s1", "(", "c0", ")", ")",
			"(", "level", "high", "s1", "(", "c0", ")", ")",
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
			"(", "level", "low", "s0", "(", "c1", ")", ")",
			"(", "level", "high", "s0", "(", "c1", ")", ")",
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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
			"(", "level", "l2", "s0", "(", "c1", ")", ")",
			"(", "level", "h2", "s0", "(", "c1", ")", ")",
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
			"(", "level", "l2", "s0", "(", "c1", ")", ")",
			"(", "level", "h2", "s0", "(", "c1", ")", ")",
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
			"(", "level", "l2", "s0", "(", "c1", ")", ")",
			"(", "level", "h2", "s0", "(", "c1", ")", ")",
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
			"(", "level", "l2", "s0", "(", "c1", ")", ")",
			"(", "level", "h2", "s0", "(", "c1", ")", ")",
			"(", "mlsconstrain", "(", "file", "dir", ")", "(", "foo", ")", "(", "eq", "l2", "h2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_constrain(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_constrain_expr_neg(CuTest *tc) {
        char *line[] = {"(", "class", "file", "(", "create", "relabelto", ")", ")",
			"(", "class", "dir", "(", "create", "relabelto", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c1", ")",
			"(", "level", "l2", "s0", "(", "c1", ")", ")",
			"(", "level", "foo", "s0", "(", "c1", ")", ")",
			"(", "mlsconstrain", "(", "file", "dir", ")", "(", "create", "relabelto", ")", "(", "eq", "l2", "32", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_constrain(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_context(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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
			"(", "roletransition", "foo_r", "bar_t", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roletrans(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_roletrans_srcdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "bar_t", ")",
			"(", "role", "foobar_r", ")", 
			"(", "roletransition", "foo_r", "bar_t", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roletrans(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_roletrans_tgtdecl_neg(CuTest *tc) {
	char *line[] = {"(", "role", "foo_r", ")",
			"(", "role", "foobar_r", ")", 
			"(", "roletransition", "foo_r", "bar_t", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roletrans(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_roletrans_resultdecl_neg(CuTest *tc) {
	char *line[] = {"(", "role", "foo_r", ")",
			"(", "type", "bar_t", ")", 
			"(", "roletransition", "foo_r", "bar_t", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_roletrans(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_typeattr(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")", 
			"(", "attribute", "bar", ")", 
			"(", "typeattribute", "foo", "bar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_typeattr(test_db, test_db->ast->root->cl_head->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_typeattr_typedecl_neg(CuTest *tc) {
	char *line[] = {"(", "attribute", "bar", ")", 
			"(", "typeattribute", "foo", "bar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_typeattr(test_db, test_db->ast->root->cl_head->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_typeattr_attrdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")", 
			"(", "typeattribute", "foo", "bar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = cil_resolve_typeattr(test_db, test_db->ast->root->cl_head->next, NULL);
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
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

void test_cil_resolve_sidcontext_named_context_sidcontextnull_neg(CuTest *tc) {
           char *line[] = {"(", "category", "c0", ")",
                        "(", "categoryorder", "(", "c0", ")", ")",
                        "(", "sensitivity", "s0", ")",
                        "(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
                        "(", "type", "blah_t", ")",
                        "(", "role", "blah_r", ")",
                        "(", "user", "blah_u", ")",
			"(", "sid", "test", ")",
                        "(", "sidcontext", "test", "(", ")", ")", NULL};

        struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        cil_resolve_senscat(test_db, test_db->ast->root->cl_head->next->next->next->next, NULL);

        int rc = cil_resolve_sidcontext(test_db, test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, NULL);
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

void test_cil_resolve_call1_level(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "l", "s0", "(", "c0", ")", ")",
			"(", "level", "h", "s0", "(", "c0", ")", ")",
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
			"(", "level", "l", "s0", "(", "c0", ")", ")",
			"(", "level", "h", "s0", "(", "c0", ")", ")",
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
	cil_resolve_name(test_db, test_db->ast->root->cl_head->next->next->next, ((struct cil_call*)test_db->ast->root->cl_head->next->next->next->data)->macro_str, CIL_SYM_MACROS, CIL_MACRO, NULL, &macro_node);
	((struct cil_call*)test_db->ast->root->cl_head->next->next->next->data)->macro = (struct cil_macro*)macro_node->data;
	free(((struct cil_call*)test_db->ast->root->cl_head->next->next->next->data)->macro_str);
	((struct cil_call*)test_db->ast->root->cl_head->next->next->next->data)->macro_str = NULL;

	((struct cil_call*)test_db->ast->root->cl_head->next->next->next->data)->macro->params->head->flavor = CIL_NETIFCON;

	int rc = cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
}

void test_cil_resolve_call1_noname_neg(CuTest *tc) {
	char *line[] = {"(", "class", "foo", "(", "read", ")", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "class", "a", ")", ")",
				"(", "class", "b", "(", "read", ")", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

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
			"(", "level", "l", "s0", "(", "c0", ")", ")",
			"(", "level", "h", "s0", "(", "c0", ")", ")",
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
			"(", "level", "l", "s0", "(", "c0", ")", ")",
			"(", "level", "h", "s0", "(", "c0", ")", ")",
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

void test_cil_resolve_call2_unknown_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "l", "s0", "(", "c0", ")", ")",
			"(", "level", "h", "s0", "(", "c0", ")", ")",
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
	CuAssertIntEquals(tc, SEPOL_OK, rc);
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
	CuAssertIntEquals(tc, SEPOL_OK, rc);
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
	int rc = cil_resolve_name_call_args((struct cil_call *)test_db->ast->root->cl_head->next->next->next->data, "a", CIL_TYPE, &test_node);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
}

void test_cil_resolve_name_call_args_extraparams(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "sensitivity", "s0", ")",
			"(", "user", "system_u", ")",
			"(", "role", "role_r", ")",
			"(", "type", "type_t", ")",
			"(", "level", "l", "s0", "(", "c0", ")", ")",
			"(", "level", "h", "s0", "(", "c0", ")", ")",
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
	int rc = cil_resolve_name_call_args((struct cil_call *)test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->data, "lvl_h", CIL_LEVEL, &test_node);
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	cil_list_item_init(&other->head->next);
	cil_list_item_init(&other->head->next->next);
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;
	other->head->next->next->next->flavor = CIL_AST_NODE;
	other->head->next->flavor = CIL_INT;
	int pass = 1;
	other->head->next->data = &pass;

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, other);	
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	cil_list_item_init(&other->head->next);
	cil_list_item_init(&other->head->next->next);
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;
	other->head->next->next->next->flavor = CIL_AST_NODE;
	other->head->next->flavor = CIL_INT;
	int pass = 1;
	other->head->next->data = &pass;

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, other);	
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	cil_list_item_init(&other->head->next);
	cil_list_item_init(&other->head->next->next);
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;
	other->head->next->next->next->flavor = CIL_AST_NODE;
	other->head->next->flavor = CIL_INT;
	int pass = 2;
	other->head->next->data = &pass;

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_tree_walk(test_db->ast->root,  __cil_resolve_ast_node_helper, NULL, NULL, other);
	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, other);	
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_call2_neg(CuTest *tc) {
	char *line[] = {"(", "type", "qaz", ")",
			"(", "class", "file", "(", "read", ")", ")",
			"(", "macro", "mm", "(", "(", "type", "a", ")", ")",
				"(", "type", "b", ")",
				"(", "allow", "a", "b", "file", "(", "read", ")", ")", ")",
			"(", "call", "mm", "(", "(", "qaz", "baz", ")", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	cil_list_item_init(&other->head->next);
	cil_list_item_init(&other->head->next->next);
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;
	other->head->next->next->next->flavor = CIL_AST_NODE;
	other->head->next->flavor = CIL_INT;
	int pass = 2;
	other->head->next->data = &pass;

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_tree_walk(test_db->ast->root,  __cil_resolve_ast_node_helper, NULL, NULL, other);
	cil_resolve_call1(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	cil_resolve_call2(test_db, test_db->ast->root->cl_head->next->next->next, NULL);
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, other);	
	CuAssertIntEquals(tc, SEPOL_ERR, rc);
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 3;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, other);	
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 3;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, other);	
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 3;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, other);	
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 3;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, other);	
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_roleallow_neg(CuTest *tc) {
	char *line[] = {"(", "role", "foo", ")", \
			"(", "roleallow", "foo", "bar", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_sensalias(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "sensitivityalias", "s0", "alias", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_sensalias_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivityalias", "s0", "alias", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head, finished, other);	
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_catalias(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")", 
			"(", "categoryalias", "c0", "red", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, finished, other);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_catalias_neg(CuTest *tc) {
	char *line[] = {"(", "categoryalias", "c0", "red", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head, finished, other);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);	
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, finished, other);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_catset_catlist_neg(CuTest *tc) {
	char *line[] = {"(", "category", "c0", ")",
			"(", "category", "c1", ")",
			"(", "categoryset", "somecats", "(", "c0", "c1", "c2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, finished, other);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_level(CuTest *tc) {
        char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c1", ")",
			"(", "sensitivitycategory", "s0", "(", "c1", ")", ")",
			"(", "level", "l2", "s0", "(", "c1", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 3;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_tree_walk(test_db->ast->root,  __cil_resolve_ast_node_helper, NULL, NULL, other);
	__cil_verify_order(test_db->catorder, test_db->ast->root, CIL_CAT);

	__cil_verify_order(test_db->dominance, test_db->ast->root, CIL_SENS);

	pass = 4;
	cil_tree_walk(test_db->ast->root, __cil_resolve_ast_node_helper, NULL, NULL, other);

	pass = 5;

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, other);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, finished, 0);
}

void test_cil_resolve_ast_node_helper_level_neg(CuTest *tc) {
        char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c1", ")",
			"(", "sensitivitycategory", "s0", "(", "c1", ")", ")",
			"(", "level", "l2", "s8", "(", "c1", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 3;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_tree_walk(test_db->ast->root,  __cil_resolve_ast_node_helper, NULL, NULL, other);
	__cil_verify_order(test_db->catorder, test_db->ast->root, CIL_CAT);

	__cil_verify_order(test_db->dominance, test_db->ast->root, CIL_SENS);

	pass = 4;
	cil_tree_walk(test_db->ast->root, __cil_resolve_ast_node_helper, NULL, NULL, other);

	pass = 5;

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, &finished, other);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, finished, 0);
}

void test_cil_resolve_ast_node_helper_constrain(CuTest *tc) {
        char *line[] = {"(", "class", "file", "(", "create", "relabelto", ")", ")",
			"(", "class", "dir", "(", "create", "relabelto", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c1", ")",
			"(", "level", "l2", "s0", "(", "c1", ")", ")",
			"(", "level", "h2", "s0", "(", "c1", ")", ")",
			"(", "mlsconstrain", "(", "file", "dir", ")", "(", "create", "relabelto", ")", "(", "eq", "l2", "h2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next, finished, other);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_constrain_neg(CuTest *tc) {
        char *line[] = {"(", "class", "file", "(", "read", ")", ")",
			"(", "class", "dir", "(", "read", ")", ")",
			"(", "sensitivity", "s0", ")",
			"(", "category", "c1", ")",
			"(", "level", "l2", "s0", "(", "c1", ")", ")",
			"(", "level", "h2", "s0", "(", "c1", ")", ")",
			"(", "mlsconstrain", "(", "file", "dir", ")", "(", "create", "relabelto", ")", "(", "eq", "l2", "h2", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next, finished, other);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_context(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con",
                        "(", "system_u", "object_r", "netif_t", "low", "high", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 3;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_tree_walk(test_db->ast->root,  __cil_resolve_ast_node_helper, NULL, NULL, other);
	__cil_verify_order(test_db->catorder, test_db->ast->root, CIL_CAT);

	__cil_verify_order(test_db->dominance, test_db->ast->root, CIL_SENS);

	pass = 4;
	cil_tree_walk(test_db->ast->root, __cil_resolve_ast_node_helper, NULL, NULL, other);

	pass = 5;

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, &finished, other);
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertIntEquals(tc, finished, 0);
}

void test_cil_resolve_ast_node_helper_context_neg(CuTest *tc) {
	char *line[] = {"(", "sensitivity", "s0", ")",
			"(", "category", "c0", ")",
			"(", "sensitivitycategory", "s0", "(", "c0", ")", ")",
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
			"(", "user", "system_u", ")",
			"(", "role", "object_r", ")",
			"(", "type", "netif_t", ")",
			"(", "context", "con",
                        "(", "system_u", "object_r", "netif_t", "llow", "high", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 3;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	cil_tree_walk(test_db->ast->root,  __cil_resolve_ast_node_helper, NULL, NULL, other);
	__cil_verify_order(test_db->catorder, test_db->ast->root, CIL_CAT);

	__cil_verify_order(test_db->dominance, test_db->ast->root, CIL_SENS);

	pass = 4;
	cil_tree_walk(test_db->ast->root, __cil_resolve_ast_node_helper, NULL, NULL, other);

	pass = 5;

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next, &finished, other);
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 4;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next, &finished, other);
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
                        "(", "sensitivitycategory", "s5", "(", "c0", "c255", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 4;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t finished = 0;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next, &finished, other);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertIntEquals(tc, 0, finished);
}

void test_cil_resolve_ast_node_helper_roletrans(CuTest *tc) {
	char *line[] = {"(", "role", "foo_r", ")",
			"(", "type", "bar_t", ")",
			"(", "role", "foobar_r", ")", 
			"(", "roletransition", "foo_r", "bar_t", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_roletrans_srcdecl_neg(CuTest *tc) {
	char *line[] = {"(", "type", "bar_t", ")",
			"(", "role", "foobar_r", ")", 
			"(", "roletransition", "foo_r", "bar_t", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_roletrans_tgtdecl_neg(CuTest *tc) {
	char *line[] = {"(", "role", "foo_r", ")",
			"(", "role", "foobar_r", ")", 
			"(", "roletransition", "foo_r", "bar_t", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_roletrans_resultdecl_neg(CuTest *tc) {
	char *line[] = {"(", "role", "foo_r", ")",
			"(", "type", "bar_t", ")",
			"(", "roletransition", "foo_r", "bar_t", "foobar_r", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_typeattr(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "attribute", "bar", ")",
			"(", "typeattribute", "foo", "bar", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_typeattr_neg(CuTest *tc) {
	char *line[] = {"(", "type", "foo", ")",
			"(", "typeattribute", "foo", "bar", ")", NULL};
		
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_typealias(CuTest *tc) {
	char *line[] = {"(", "block", "foo", 
				"(", "typealias", ".foo.test", "type_t", ")", 
				"(", "type", "test", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->cl_head, finished, other);	
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_typealias_notype_neg(CuTest *tc) {
	char *line[] = {"(", "block", "bar", 
				"(", "typealias", ".bar.test", "type_t", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->cl_head, finished, other);	
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_avrule_src_nores_neg(CuTest *tc) {
	char *line[] = {"(", "allow", "test", "foo", "bar", "(", "read", "write", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head, finished, other);	
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_avrule_tgt_nores_neg(CuTest *tc) {
	char *line[] = {"(", "type", "test", ")", 
			"(", "allow", "test", "foo", "bar", "(", "read", "write", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_avrule_class_nores_neg(CuTest *tc) {
	char *line[] = {"(", "type", "test", ")", 
			"(", "type", "foo", ")", 
			"(", "allow", "test", "foo", "bar", "(", "read", "write", ")", ")", NULL};
	
	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
}

void test_cil_resolve_ast_node_helper_avrule_datum_null_neg(CuTest *tc) {
	char *line[] = {"(", "class", "bar", "(", "read", "write", "open", ")", ")", \
	                "(", "type", "test", ")", "(", "type", "foo", ")", \
	                "(", "allow", "test", "foo", "bar", "(","fake", ")", ")", NULL};

	struct cil_tree *test_tree;
	gen_test_tree(&test_tree, line);

	struct cil_db *test_db;
	cil_db_init(&test_db);
	
	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertPtrEquals(tc, NULL, finished);
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertPtrEquals(tc, NULL, finished);
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_OK, rc);
	CuAssertPtrEquals(tc, NULL, finished);
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

	struct cil_list *other;
	cil_list_init(&other);
	cil_list_item_init(&other->head);
	other->head->data = test_db;
	other->head->flavor = CIL_DB;
	cil_list_item_init(&other->head->next);
	other->head->next->flavor = CIL_INT;
	int pass = 5;
	other->head->next->data = &pass;
	cil_list_item_init(&other->head->next->next);
	other->head->next->next->data = NULL;
	cil_list_item_init(&other->head->next->next->next);
	other->head->next->next->next->data = NULL;

	uint32_t *finished = NULL;
	
	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next, finished, other);	
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
	CuAssertPtrEquals(tc, NULL, finished);
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

	struct cil_list *cil_l;
	cil_list_init(&cil_l);

	uint32_t *finished = NULL;

	cil_list_item_init(&cil_l->head);
	cil_l->head->data = test_db;
	cil_l->head->flavor = CIL_DB;
	cil_list_item_init(&cil_l->head->next);
	cil_l->head->next->flavor = CIL_INT;
	int pass = 5;
	cil_l->head->next->data = &pass;
	cil_list_item_init(&cil_l->head->next->next);
	cil_l->head->next->next->data = NULL;
	cil_list_item_init(&cil_l->head->next->next->next);
	cil_l->head->next->next->next->data = NULL;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, finished, cil_l);
	CuAssertPtrEquals(tc, NULL, finished);
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

	struct cil_list *cil_l;
	cil_list_init(&cil_l);

	uint32_t *finished = NULL;

	cil_list_item_init(&cil_l->head);
	cil_l->head->data = test_db;
	cil_l->head->flavor = CIL_DB;
	cil_list_item_init(&cil_l->head->next);
	cil_l->head->next->flavor = CIL_INT;
	int pass = 5;
	cil_l->head->next->data = &pass;
	cil_list_item_init(&cil_l->head->next->next);
	cil_l->head->next->next->data = NULL;
	cil_list_item_init(&cil_l->head->next->next->next);
	cil_l->head->next->next->next->data = NULL;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, finished, cil_l);
	CuAssertPtrEquals(tc, NULL, finished);
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

	struct cil_list *cil_l;
	cil_list_init(&cil_l);

	uint32_t *finished = NULL;

	cil_list_item_init(&cil_l->head);
	cil_l->head->data = test_db;
	cil_l->head->flavor = CIL_DB;
	cil_list_item_init(&cil_l->head->next);
	cil_l->head->next->flavor = CIL_INT;
	int pass = 5;
	cil_l->head->next->data = &pass;
	cil_list_item_init(&cil_l->head->next->next);
	cil_l->head->next->next->data = NULL;
	cil_list_item_init(&cil_l->head->next->next->next);
	cil_l->head->next->next->next->data = NULL;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, finished, cil_l);
	CuAssertPtrEquals(tc, NULL, finished);
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

	struct cil_list *cil_l;
	cil_list_init(&cil_l);

	uint32_t *finished = NULL;

	cil_list_item_init(&cil_l->head);
	cil_l->head->data = test_db;
	cil_l->head->flavor = CIL_DB;
	cil_list_item_init(&cil_l->head->next);
	cil_l->head->next->flavor = CIL_INT;
	int pass = 5;
	cil_l->head->next->data = &pass;
	cil_list_item_init(&cil_l->head->next->next);
	cil_l->head->next->next->data = NULL;
	cil_list_item_init(&cil_l->head->next->next->next);
	cil_l->head->next->next->next->data = NULL;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);
	
	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, finished, cil_l);
	CuAssertPtrEquals(tc, NULL, finished);
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

	struct cil_list *cil_l;
	cil_list_init(&cil_l);

	uint32_t *finished = NULL;

	cil_list_item_init(&cil_l->head);
	cil_l->head->data = test_db;
	cil_l->head->flavor = CIL_DB;
	cil_list_item_init(&cil_l->head->next);
	cil_l->head->next->flavor = CIL_INT;
	int pass = 5;
	cil_l->head->next->data = &pass;
	cil_list_item_init(&cil_l->head->next->next);
	cil_l->head->next->next->data = NULL;
	cil_list_item_init(&cil_l->head->next->next->next);
	cil_l->head->next->next->next->data = NULL;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, finished, cil_l);
	CuAssertPtrEquals(tc, NULL, finished);
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

	struct cil_list *cil_l;
	cil_list_init(&cil_l);

	uint32_t *finished = NULL;

	cil_list_item_init(&cil_l->head);
	cil_l->head->data = test_db;
	cil_l->head->flavor = CIL_DB;
	cil_list_item_init(&cil_l->head->next);
	cil_l->head->next->flavor = CIL_INT;
	int pass = 5;
	cil_l->head->next->data = &pass;
	cil_list_item_init(&cil_l->head->next->next);
	cil_l->head->next->next->data = NULL;
	cil_list_item_init(&cil_l->head->next->next->next);
	cil_l->head->next->next->next->data = NULL;

	cil_build_ast(test_db, test_tree->root, test_db->ast->root);

	int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, finished, cil_l);
	CuAssertPtrEquals(tc, NULL, finished);
	CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
}

void test_cil_resolve_ast_node_helper_netifcon(CuTest *tc) {
	char *line[] = {"(", "context", "if_default", "(", "system_u", "object_r", "etc_t", "low", "high", ")", ")",
			"(", "context", "packet_default", "(", "system_u", "object_r", "etc_t", "low", "high", ")", ")", 
			"(", "netifcon", "eth0", "if_default", "packet_default", ")", NULL};
        
	struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

        struct cil_list *other;
        cil_list_init(&other);
        cil_list_item_init(&other->head);
        other->head->data = test_db;
        other->head->flavor = CIL_DB;
        cil_list_item_init(&other->head->next);
        other->head->next->flavor = CIL_INT;
        int pass = 5;
        other->head->next->data = &pass;
        cil_list_item_init(&other->head->next->next);
        other->head->next->next->data = NULL;
        cil_list_item_init(&other->head->next->next->next);
        other->head->next->next->next->data = NULL;

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, other);
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

        struct cil_list *other;
        cil_list_init(&other);
        cil_list_item_init(&other->head);
        other->head->data = test_db;
        other->head->flavor = CIL_DB;
        cil_list_item_init(&other->head->next);
        other->head->next->flavor = CIL_INT;
        int pass = 5;
        other->head->next->data = &pass;
        cil_list_item_init(&other->head->next->next);
        other->head->next->next->data = NULL;
        cil_list_item_init(&other->head->next->next->next);
        other->head->next->next->next->data = NULL;

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, other);
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
			"(", "level", "low", "s0", "(", "c0", ")", ")",
			"(", "level", "high", "s0", "(", "c0", ")", ")",
			"(", "sid", "test", "(", "blah_u", "blah_r", "blah_t", "low", "high", ")", ")", NULL};
        
	struct cil_tree *test_tree;
        gen_test_tree(&test_tree, line);

        struct cil_db *test_db;
        cil_db_init(&test_db);

        struct cil_list *other;
        cil_list_init(&other);
        cil_list_item_init(&other->head);
        other->head->data = test_db;
        other->head->flavor = CIL_DB;
        cil_list_item_init(&other->head->next);
        other->head->next->flavor = CIL_INT;
        int pass = 5;
        other->head->next->data = &pass;
        cil_list_item_init(&other->head->next->next);
        other->head->next->next->data = NULL;
        cil_list_item_init(&other->head->next->next->next);
        other->head->next->next->next->data = NULL;

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next->next->next, &finished, other);
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

        struct cil_list *other;
        cil_list_init(&other);
        cil_list_item_init(&other->head);
        other->head->data = test_db;
        other->head->flavor = CIL_DB;
        cil_list_item_init(&other->head->next);
        other->head->next->flavor = CIL_INT;
        int pass = 5;
        other->head->next->data = &pass;
        cil_list_item_init(&other->head->next->next);
        other->head->next->next->data = NULL;
        cil_list_item_init(&other->head->next->next->next);
        other->head->next->next->next->data = NULL;

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next->next->next->next->next->next, &finished, other);
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

        struct cil_list *other;
        cil_list_init(&other);
        cil_list_item_init(&other->head);
        other->head->data = test_db;
        other->head->flavor = CIL_DB;
        cil_list_item_init(&other->head->next);
        other->head->next->flavor = CIL_INT;
        int pass = 4;
        other->head->next->data = &pass;
        cil_list_item_init(&other->head->next->next);
        other->head->next->next->data = NULL;
        cil_list_item_init(&other->head->next->next->next);
        other->head->next->next->next->data = NULL;

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next->next, &finished, other);
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

        struct cil_list *other;
        cil_list_init(&other);
        cil_list_item_init(&other->head);
        other->head->data = test_db;
        other->head->flavor = CIL_DB;
        cil_list_item_init(&other->head->next);
        other->head->next->flavor = CIL_INT;
        int pass = 4;
        other->head->next->data = &pass;
        cil_list_item_init(&other->head->next->next);
        other->head->next->next->data = NULL;
        cil_list_item_init(&other->head->next->next->next);
        other->head->next->next->next->data = NULL;

        uint32_t finished = 0;

        cil_build_ast(test_db, test_tree->root, test_db->ast->root);

        int rc = __cil_resolve_ast_node_helper(test_db->ast->root->cl_head->next, &finished, other);
        CuAssertIntEquals(tc, SEPOL_ENOENT, rc);
        CuAssertIntEquals(tc, 0, finished);
}
