/* The MIT License (MIT)
 * Copyright (c) 2016 Thomas Bertauld <thomas.bertauld@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "list.h"
#include <stdlib.h>
#include <stdio.h>

static inline char list_elt_equals(const void *elt1, const void *elt2, const size_t size) {
	return (memcmp(elt1, elt2, size) == 0);
}

//------------------------------------------------------------------------------------

void list_push(list_t **l, const void *elt, const size_t size) {
	if (l==NULL) {
		fprintf(stderr, "list_push error : invalid list reference.\n");
		return;
	} else if (elt == NULL) {
		fprintf(stderr, "list_push error : invalid element.\n");
		return;
	}

	void *elt_copy = malloc(size);
	memcpy(elt_copy, elt, size);

	list_t *temp = *l;
	(*l) = (list_t*)malloc(sizeof(list_t));
	(*l)->val = elt_copy;
	(*l)->next = temp;

	return;
}

//------------------------------------------------------------------------------------

void *list_pop(list_t **l) {
	if (l==NULL) {
		fprintf(stderr, "list_push error : invalid list reference.\n");
		return NULL;
	}

	if (*l == NULL) {
		fprintf(stderr, "list_pop warning : nothing to pop (empty list).\n");
		return NULL;
	}
	
	void *res = (*l)->val;
	list_t *temp = (*l)->next;
	free(*l);
	*l = temp;

	return res;
}

//------------------------------------------------------------------------------------

void *list_search(list_t ** l, const void *elt, const size_t size) {
	if (l==NULL) {
		fprintf(stderr, "list_search error : invalid list reference.\n");
		return NULL;
	}

	if (*l == NULL) {
		fprintf(stderr, "list_search warning : empty list.\n");
		return NULL;
	}

	if (elt == NULL) {
		fprintf(stderr, "list_search error : invalid element reference.\n");
		return NULL;
	}

	void *res = NULL;
	list_t *prev = *l;
	list_t *tmp = *l;
	while (tmp != NULL) {	
		if (list_elt_equals(elt, tmp->val, size)) {
			res = tmp->val;
			if (tmp == *l) {
				tmp = (*l)->next;
				free(*l);
				*l = tmp;
				break;
			}
			prev->next = tmp->next;
			free(tmp);
			break;
		}
		prev = tmp;
		tmp = tmp->next;
	}

	return res;
}

	

//------------------------------------------------------------------------------------

void list_destroy(list_t **l) {
	if (l==NULL) {
		fprintf(stderr, "list_destroy error : invalid list reference.\n");
		return;
	}

	if (*l == NULL) {
		fprintf(stderr, "list_destroy warning : empty list.\n");
		return;
	}

	while (*l != NULL) {
		list_t *temp = (*l)->next;
		free((*l)->val);
		free(*l);
		(*l) = temp;
	}

	*l = NULL;

	return;
}

//------------------------------------------------------------------------------------

char list_has_next(list_t *l) {
	return (l != NULL && (l->next != NULL));
}

//------------------------------------------------------------------------------------

#ifdef LIST_TEST
int main(int argc, char **argv) {
	
	list_t *test = NULL;
	int *i = malloc(sizeof(int));
	int *search = malloc(sizeof(int));
	*search = 32;
	*i = 12;
	list_push(&test, i , sizeof(int));
	*i = 1;
	list_push(&test, i , sizeof(int));
	*i = 32;
	list_push(&test, i , sizeof(int));
	*i = 15;
	list_push(&test, i , sizeof(int));
	*i = 43;
	list_push(&test, i , sizeof(int));
	free(i);

	list_t *temp = test;
	fprintf(stderr, "[%i", *((int *)test->val));
	test = test->next;
	while (test != NULL) {
		fprintf(stderr, "; %i", *((int *)test->val));
		test = test->next;
	}
	fprintf(stderr, "]\n");
	test = temp;

	i = (int *)list_search(&test, search, sizeof(int));
	fprintf(stderr, "Value searched : 32, found value : %i\n", *i);
	free(i);

	temp = test;
	fprintf(stderr, "[%i", *((int *)test->val));
	test = test->next;
	while (test != NULL) {
		fprintf(stderr, "; %i", *((int *)test->val));
		test = test->next;
	}
	fprintf(stderr, "]\n");
	test = temp;

	*search = 43;
	i = (int *)list_search(&test, search, sizeof(int));
	fprintf(stderr, "Value searched : 43, found value : %i\n", *i);

	temp = test;
	fprintf(stderr, "[%i", *((int *)test->val));
	test = test->next;
	while (test != NULL) {
		fprintf(stderr, "; %i", *((int *)test->val));
		test = test->next;
	}
	fprintf(stderr, "]\n");
	test = temp;

	list_destroy(&test);
	free(i);
	free(search);

	return EXIT_SUCCESS;
}
#endif // LIST_TEST
