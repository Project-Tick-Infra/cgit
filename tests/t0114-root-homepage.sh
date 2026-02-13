#!/bin/sh

test_description='Check root external links'
. ./setup.sh

test_expect_success 'index has configured root links' '
	cgit_url "" >tmp &&
	grep "https://projecttick.org" tmp &&
	grep ">Project Tick<" tmp &&
	grep "https://github.com/example" tmp &&
	grep ">GitHub<" tmp &&
	grep "https://gitlab.com/example" tmp &&
	grep ">GitLab<" tmp &&
	grep "https://codeberg.org/example" tmp &&
	grep ">Codeberg<" tmp
'

test_expect_success 'root pages keep root links' '
	cgit_query "p=coc" >tmp &&
	grep "https://projecttick.org" tmp &&
	grep ">Project Tick<" tmp &&
	grep "https://github.com/example" tmp &&
	grep ">GitHub<" tmp &&
	grep "https://gitlab.com/example" tmp &&
	grep ">GitLab<" tmp &&
	grep "https://codeberg.org/example" tmp &&
	grep ">Codeberg<" tmp
'

test_done
