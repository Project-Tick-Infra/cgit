/* ui-summary.c: functions for generating repo summary page
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 * Copyright (C) 2026 Project Tick
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#define USE_THE_REPOSITORY_VARIABLE

#include "cgit.h"
#include "ui-summary.h"
#include "html.h"
#include "ui-blob.h"
#include "ui-log.h"
#include "ui-plain.h"
#include "ui-refs.h"
#include "ui-shared.h"

static int urls;
#define MAX_METADATA_BYTES (64 * 1024)

static char *trim_whitespace(const char *start, const char *end)
{
	while (start < end && isspace((unsigned char)*start))
		start++;
	while (end > start && isspace((unsigned char)end[-1]))
		end--;
	if (end <= start)
		return NULL;
	return xstrndup(start, end - start);
}

static char *find_spdx_identifier(const char *buf, size_t size)
{
	static const char *keys[] = {
		"SPDX-License-Identifier:",
		"SPDX-License-Expression:",
	};
	const char *line = buf;
	const char *end = buf + size;

	while (line && line < end) {
		const char *eol = strchrnul(line, '\n');
		size_t len = eol - line;
		size_t i;

		for (i = 0; i < ARRAY_SIZE(keys); i++) {
			const char *value;

			if (len < strlen(keys[i]))
				continue;
			if (!skip_prefix(line, keys[i], &value))
				continue;
			return trim_whitespace(value, eol);
		}

		if (!*eol)
			break;
		line = eol + 1;
	}
	return NULL;
}

static char *detect_spdx_for_path(const char *path)
{
	char *buf = NULL;
	unsigned long size = 0;
	char *spdx = NULL;

	if (!path || !ctx.qry.head)
		return NULL;
	if (cgit_ref_read_file(path, ctx.qry.head, 1, &buf, &size))
		return NULL;
	if (size > MAX_METADATA_BYTES)
		goto out;
	spdx = find_spdx_identifier(buf, size);

out:
	free(buf);
	return spdx;
}

static char *find_first_repo_path(const char **candidates, size_t nr)
{
	size_t i;
	const char *ref = ctx.qry.head;

	if (!ref)
		return NULL;

	for (i = 0; i < nr; i++) {
		if (cgit_ref_path_exists(candidates[i], ref, 1))
			return xstrdup(candidates[i]);
	}
	return NULL;
}

static void print_repo_file_link(const char *path)
{
	if (!path)
		return;
	cgit_tree_link(path, NULL, "ls-blob", ctx.qry.head, ctx.qry.head, path);
}

static void print_repo_metadata(void)
{
	static const char *license_candidates[] = {
		"LICENSE",
		"LICENSE.md",
		"LICENSE.txt",
		"LICENCE",
		"LICENCE.md",
		"LICENCE.txt",
		"COPYING",
		"COPYING.md",
		"COPYING.txt",
	};
	static const char *codeowners_candidates[] = {
		"CODEOWNERS",
		".github/CODEOWNERS",
		"docs/CODEOWNERS",
		".gitlab/CODEOWNERS",
	};
	static const char *maintainers_candidates[] = {
		"MAINTAINERS",
		"MAINTAINERS.md",
		"MAINTAINERS.txt",
		"MAINTAINER",
		"MAINTAINER.md",
		"MAINTAINER.txt",
	};
	char *license_path = find_first_repo_path(license_candidates,
						  ARRAY_SIZE(license_candidates));
	char *license_spdx = license_path ? detect_spdx_for_path(license_path) : NULL;
	char *codeowners_path = find_first_repo_path(codeowners_candidates,
						     ARRAY_SIZE(codeowners_candidates));
	char *maintainers_path = find_first_repo_path(maintainers_candidates,
						      ARRAY_SIZE(maintainers_candidates));

	if (!license_path && !codeowners_path && !maintainers_path)
		goto cleanup;

	html("<div class='summary-metadata'>");
	html("<table summary='repository metadata' class='list nowrap'>");
	html("<tr class='nohover'>");
	html("<th class='left'>Metadata</th>");
	html("<th class='left'></th>");
	html("</tr>\n");

	if (license_path) {
		html("<tr><td class='left'>License</td><td class='left'>");
		if (license_spdx) {
			html_txt(license_spdx);
			html(" (");
			print_repo_file_link(license_path);
			html(")");
		} else {
			print_repo_file_link(license_path);
		}
		html("</td></tr>\n");
	}

	if (codeowners_path) {
		html("<tr><td class='left'>Codeowners</td><td class='left'>");
		print_repo_file_link(codeowners_path);
		html("</td></tr>\n");
	}

	if (maintainers_path) {
		html("<tr><td class='left'>Maintainers</td><td class='left'>");
		print_repo_file_link(maintainers_path);
		html("</td></tr>\n");
	}

	html("</table>");
	html("</div>");

cleanup:
	free(license_spdx);
	free(license_path);
	free(codeowners_path);
	free(maintainers_path);
}

static unsigned long ref_age(const struct refinfo *ref)
{
	if (!ref || !ref->object)
		return 0;
	switch (ref->object->type) {
	case OBJ_TAG:
		return ref->tag ? ref->tag->tagger_date : 0;
	case OBJ_COMMIT:
		return ref->commit ? ref->commit->committer_date : 0;
	default:
		return 0;
	}
}

static int cmp_ref_age_desc(const void *a, const void *b)
{
	const struct refinfo *r1 = *(const struct refinfo **)a;
	const struct refinfo *r2 = *(const struct refinfo **)b;
	unsigned long age1 = ref_age(r1);
	unsigned long age2 = ref_age(r2);

	if (age1 == age2)
		return 0;
	return (age1 > age2) ? -1 : 1;
}

static char *get_latest_tag_name(void)
{
	struct reflist list;
	char *result = NULL;

	memset(&list, 0, sizeof(list));
	refs_for_each_tag_ref(get_main_ref_store(the_repository),
			      cgit_refs_cb, &list);
	if (!list.count)
		goto cleanup;
	qsort(list.refs, list.count, sizeof(*list.refs), cmp_ref_age_desc);
	result = xstrdup(list.refs[0]->refname);

cleanup:
	cgit_free_reflist_inner(&list);
	return result;
}

static void print_repo_badges(void)
{
	struct string_list_item *item;

	for_each_string_list_item(item, &ctx.repo->badges) {
		html("<span class='badge'>");
		if (item->util) {
			html("<a href='");
			html_attr((const char *)item->util);
			html("'>");
		}
		html("<img class='badge' alt='badge' src='");
		html_attr(item->string);
		html("'/>");
		if (item->util)
			html("</a>");
		html("</span>");
	}
}

static void print_repo_overview(void)
{
	char *latest_tag = get_latest_tag_name();
	int has_badges = ctx.repo->badges.nr > 0;

	if (!ctx.repo->defbranch && !latest_tag && !has_badges)
		return;

	html("<div class='summary-overview'>");
	html("<table summary='repository overview' class='list nowrap'>");
	html("<tr class='nohover'><th class='left'>Overview</th>");
	html("<th class='left'></th></tr>\n");

	if (ctx.repo->defbranch) {
		html("<tr><td class='left'>Default branch</td><td class='left'>");
		cgit_log_link(ctx.repo->defbranch, NULL, NULL,
			      ctx.repo->defbranch, NULL, NULL,
			      0, NULL, NULL, 0, 0);
		html("</td></tr>\n");
	}

	if (latest_tag) {
		html("<tr><td class='left'>Latest tag</td><td class='left'>");
		cgit_tag_link(latest_tag, NULL, NULL, latest_tag);
		html("</td></tr>\n");
	}

	if (has_badges) {
		html("<tr><td class='left'>Badges</td><td class='left'>");
		print_repo_badges();
		html("</td></tr>\n");
	}

	html("</table>");
	html("</div>");

	free(latest_tag);
}

static void print_url(const char *url)
{
	int columns = 3;

	if (ctx.repo->enable_log_filecount)
		columns++;
	if (ctx.repo->enable_log_linecount)
		columns++;

	if (urls++ == 0) {
		htmlf("<tr class='nohover'><td colspan='%d'>&nbsp;</td></tr>", columns);
		htmlf("<tr class='nohover'><th class='left' colspan='%d'>Clone</th></tr>\n", columns);
	}

	htmlf("<tr><td colspan='%d'><a rel='vcs-git' href='", columns);
	html_url_path(url);
	html("' title='");
	html_attr(ctx.repo->name);
	html(" Git repository'>");
	html_txt(url);
	html("</a></td></tr>\n");
}

void cgit_print_summary(void)
{
	int columns = 3;

	if (ctx.repo->enable_log_filecount)
		columns++;
	if (ctx.repo->enable_log_linecount)
		columns++;

	cgit_print_layout_start();
	print_repo_overview();
	print_repo_metadata();
	html("<table summary='repository info' class='list nowrap'>");
	cgit_print_branches(ctx.cfg.summary_branches);
	htmlf("<tr class='nohover'><td colspan='%d'>&nbsp;</td></tr>", columns);
	cgit_print_tags(ctx.cfg.summary_tags);
	if (ctx.cfg.summary_log > 0) {
		htmlf("<tr class='nohover'><td colspan='%d'>&nbsp;</td></tr>", columns);
		cgit_print_log(ctx.qry.head, 0, ctx.cfg.summary_log, NULL,
			       NULL, NULL, 0, 0, 0);
	}
	urls = 0;
	cgit_add_clone_urls(print_url);
	html("</table>");
	cgit_print_layout_end();
}

/* The caller must free the return value. */
static char* append_readme_path(const char *filename, const char *ref, const char *path)
{
	char *file, *base_dir, *full_path, *resolved_base = NULL, *resolved_full = NULL;
	/* If a subpath is specified for the about page, make it relative
	 * to the directory containing the configured readme. */

	file = xstrdup(filename);
	base_dir = dirname(file);
	if (!strcmp(base_dir, ".") || !strcmp(base_dir, "..")) {
		if (!ref) {
			free(file);
			return NULL;
		}
		full_path = xstrdup(path);
	} else
		full_path = fmtalloc("%s/%s", base_dir, path);

	if (!ref) {
		resolved_base = realpath(base_dir, NULL);
		resolved_full = realpath(full_path, NULL);
		if (!resolved_base || !resolved_full || !starts_with(resolved_full, resolved_base)) {
			free(full_path);
			full_path = NULL;
		}
	}

	free(file);
	free(resolved_base);
	free(resolved_full);

	return full_path;
}

void cgit_print_repo_readme(const char *path)
{
	char *filename, *ref, *mimetype;
	int free_filename = 0;

	mimetype = get_mimetype_for_filename(path);
	if (mimetype && (!strncmp(mimetype, "image/", 6) || !strncmp(mimetype, "video/", 6))) {
		ctx.page.mimetype = mimetype;
		ctx.page.charset = NULL;
		cgit_print_plain();
		free(mimetype);
		return;
	}
	free(mimetype);

	cgit_print_layout_start();
	if (ctx.repo->readme.nr == 0)
		goto done;

	filename = ctx.repo->readme.items[0].string;
	ref = ctx.repo->readme.items[0].util;

	if (path) {
		free_filename = 1;
		filename = append_readme_path(filename, ref, path);
		if (!filename)
			goto done;
	}

	/* Print the calculated readme, either from the git repo or from the
	 * filesystem, while applying the about-filter.
	 */
	html("<div id='summary'>");
	cgit_open_filter(ctx.repo->about_filter, filename);
	if (ref)
		cgit_print_file(filename, ref, 1);
	else
		html_include(filename);
	cgit_close_filter(ctx.repo->about_filter);

	html("</div>");
	if (free_filename)
		free(filename);

done:
	cgit_print_layout_end();
}
