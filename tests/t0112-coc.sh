#!/bin/sh

test_description='Check top-level CoC page'
. ./setup.sh

test_expect_success 'index has coc tab' '
	cgit_url "" >tmp &&
	grep "p=coc" tmp
'

test_expect_success 'generate top-level coc page' '
	cgit_query "p=coc" >tmp
'

test_expect_success 'find coc content' '
	grep "site code of conduct" tmp
'

test_expect_success 'repo coc renders top-level coc page' '
	cgit_url "foo/coc" >tmp &&
	grep "site code of conduct" tmp
'

test_done
