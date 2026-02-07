/* cgit.js: javacript functions for cgit
 *
 * Copyright (C) 2006-2018 cgit Development Team <cgit@lists.zx2c4.com>
 * Copyright (C) 2026 Project Tick
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

(function () {

/* This follows the logic and suffixes used in ui-shared.c */

var age_classes = [ "age-mins", "age-hours", "age-days",    "age-weeks",    "age-months",    "age-years" ];
var age_suffix =  [ "min.",     "hours",     "days",        "weeks",        "months",        "years",         "years" ];
var age_next =    [ 60,         3600,        24 * 3600,     7 * 24 * 3600,  30 * 24 * 3600,  365 * 24 * 3600, 365 * 24 * 3600 ];
var age_limit =   [ 7200,       24 * 7200,   7 * 24 * 7200, 30 * 24 * 7200, 365 * 25 * 7200, 365 * 25 * 7200 ];
var update_next = [ 10,         5 * 60,      1800,          24 * 3600,      24 * 3600,       24 * 3600,       24 * 3600 ];

function render_age(e, age) {
	var t, n;

	for (n = 0; n < age_classes.length; n++)
		if (age < age_limit[n])
			break;

	t = Math.round(age / age_next[n]) + " " + age_suffix[n];

	if (e.textContent != t) {
		e.textContent = t;
		if (n == age_classes.length)
			n--;
		if (e.className != age_classes[n])
			e.className = age_classes[n];
	}
}

function aging() {
	var n, next = 24 * 3600,
	    now_ut = Math.round((new Date().getTime() / 1000));

	for (n = 0; n < age_classes.length; n++) {
		var m, elems = document.getElementsByClassName(age_classes[n]);

		if (elems.length && update_next[n] < next)
			next = update_next[n];

		for (m = 0; m < elems.length; m++) {
			var age = now_ut - elems[m].getAttribute("data-ut");

			render_age(elems[m], age);
		}
	}

	/*
	 * We only need to come back when the age might have changed.
	 * Eg, if everything is counted in hours already, once per
	 * 5 minutes is accurate enough.
	 */

	window.setTimeout(aging, next * 1000);
}

document.addEventListener("DOMContentLoaded", function() {
	/* we can do the aging on DOM content load since no layout dependency */
	aging();

	var treeFilter = document.getElementById("tree-filter");
	if (treeFilter) {
		var table = treeFilter.parentNode.nextElementSibling;
		var rows = table ? table.querySelectorAll("tr[data-name]") : [];
		var count = document.getElementById("tree-filter-count");

		function updateTreeFilter() {
			var q = treeFilter.value.toLowerCase();
			var shown = 0;
			var total = rows.length;

			for (var i = 0; i < rows.length; i++) {
				var name = rows[i].getAttribute("data-name") || "";
				var path = rows[i].getAttribute("data-path") || "";
				var hay = (name + " " + path).toLowerCase();
				if (!q || hay.indexOf(q) !== -1) {
					rows[i].style.display = "";
					shown++;
				} else {
					rows[i].style.display = "none";
				}
			}
			if (count) {
				if (!q) {
					count.textContent = "";
				} else {
					count.textContent = shown + " / " + total;
				}
			}
		}

		treeFilter.addEventListener("input", updateTreeFilter, false);
		updateTreeFilter();
	}
}, false);

})();
