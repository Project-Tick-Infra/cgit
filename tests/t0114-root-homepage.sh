#!/bin/sh

test_description='Check root homepage link'
. ./setup.sh

test_expect_success 'index has configured root homepage link' '
	cgit_url "" >tmp &&
	grep "https://github.com/example" tmp &&
	grep ">GitHub<" tmp
'

test_expect_success 'root pages keep root homepage link' '
	cgit_query "p=coc" >tmp &&
	grep "https://github.com/example" tmp &&
	grep ">GitHub<" tmp
'

test_done
