extends SceneTree

const DOCUMENT: String = """
<!doctype html><html><head>
<style id="leading_styles">
  body { margin: 0; }
  .probe { display: block; width: 10px; height: 4px; }
  .from_link { width: 12px; }
  .order { width: 12px; }
  .import_order { width: 12px; }
  .link_media { width: 12px; }
  .after_link { width: 12px; }
  .reimport_order { width: 12px; }
  .stylesheet_move { width: 21px; }
</style>
<link id="main_sheet" rel="  StYlEsHeEt&#9;next " href="styles/sheet.css">
<link rel="styleSheet" href="styles/media.css" media="(min-width: 400px)">
<link rel="not-stylesheet" href="styles/rejected.css">
<link rel="stylesheet alternate" href="styles/rejected.css" title="Alternate">
<link id="typed_sheet" rel="stylesheet" href="styles/typed.css" type="text/plain">
<style>
  .after_link { width: 34px; }
  #specific { width: 32px; }
  .tie { width: 12px; }
  .rule_important { width: 36px !important; }
  .spaced_important { width: 54px ! /* comment */ ImPoRtAnT; }
  .spaced_important { width: 13px; }
  #invalid_deferred { width: 55px; width: var(--size) ! invalid; }
  #unused_fallback { width: 10px; width: var(--size, calc()); }
  #spaced_shorthand { width: 15px; border: var(--border) ! important; }
  #spaced_shorthand { border-left-width: 10px; }
  .baseline_var { text-align: left; }
  #unlayered_rollback, #unlayered_var_rollback { width: 99px !important; }
  #unlayered_custom { --Width: 99px; }
  #pseudo_weight::before { content: "PSEUDO"; }
  .pseudo_weight:before { content: "WRONG"; }
  i:where(.pseudo_weight)::after { content: "TWO"; }
  :where(i.pseudo_weight)::after { content: "ONE"; }
  #important_specific { width: 38px !important; }
  #normal_specific { width: 12px; }
  .repeat { width: 12px; width: 42px; width: bogus; }
  .oversized { width: 25px; padding: 3px; }
  @supports (background-clip: content-box, padding-box) { #background_layers { width: 53px; } }
  #var_specific { --Width: 43px; }
  .var_important { --Width: 44px !important; }
  .inherit_var { --Width: 45px; }
  .shorthand { width: 20px; border: var(--border) !important; }
  .shorthand { border-left-width: 9px; }
  .invalid_var { width: 42px; width: var(--bad); }
  .ignored_var { width: var(--bad); }
  :is(.is_case, #absent) { width: 46px; }
  .not_case:not(#absent, .absent) { width: 47px; }
  .nth_case:nth-child(1 of .nth_case, #absent) { width: 48px; }
  .nth_last_case:nth-last-child(1 of .nth_last_case, #absent) { width: 49px; }
  #pseudo::before { content: "SPECIFIC"; }
  #pseudo::after { content: var(--after); }
  .pseudo::after { content: "IMPORTANT" !important; }
  #pseudo_tie:before { content: "DOCUMENT"; }
  #pseudo_reset::before, #pseudo_reset::after { content: "REMOVED_GENERATED_TEXT"; }
  #pseudo_host::before { content: "HOST"; }
  #pseudo_host > #pseudo_child::before { content: "CHILD"; }
  #pseudo_host::before > #pseudo_child { content: "INVALID_ANCESTOR"; }
  #pseudo_host::before + #pseudo_sibling { width: 99px; }
  #pseudo_host::before::after { content: "INVALID_CHAIN"; }
  ::before { color: #123456; }
  section::before { content: ""; }
  section::after { content: ""; }
  #aligned { text-align: left; }
  #custom_rollback { --base: 86px; }
  .computed_box { padding: 3px; width: 25px; }
  .computed_box { padding: var(--padding); }
  #valid_box { padding-left: 7px; }
  @media (min-width: 400px) { #specific { width: 50px; } }
</style>
<style type="">.style_type { width: 65px; }</style>
<style type="TeXt/CsS">.style_type { width: 66px; }</style>
<style id="inactive_style" type="text/plain">#style_type { width: 67px !important; }</style>
</head><body>
<div id="from_link" class="probe from_link"></div>
<div id="style_type" class="probe style_type"></div>
<div id="order" class="probe order"></div>
<div id="import_order" class="probe import_order"></div>
<div id="invalid_nested" class="probe nested_deferred" style="--nested-width: bogus"></div>
<div id="after_link" class="probe after_link"></div>
<div id="reimport_order" class="probe reimport_order"></div>
<div id="stylesheet_move" class="probe stylesheet_move"></div>
<div id="link_type" class="probe"></div>
<div id="link_media" class="probe link_media"></div>
<div id="specific" class="probe specific"></div>
<div id="tie" class="probe tie"></div>
<div id="inline_normal" class="probe tie" style="width: 35px"></div>
<div id="rule_important" class="probe rule_important" style="width: 12px"></div>
<div id="inline_important" class="probe rule_important" style="width: 37px !important; width: 12px"></div>
<div id="spaced_important" class="probe spaced_important" style="width: 12px"></div>
<div id="spaced_inline" class="probe spaced_important" style="width: 56px ! important; width: 12px"></div>
<div id="invalid_deferred" class="probe" style="--size: 93px"></div>
<div id="unused_fallback" class="probe" style="--size: 57px"></div>
<div id="spaced_shorthand" class="probe" style="--border: 2px solid black; border-right-width: 5px ! important"></div>
<div id="inline_important_vs_normal" class="probe tie" style="width: 39px !important"></div>
<div id="important_specific" class="probe important_specific"></div>
<div id="normal_specific" class="probe normal_specific"></div>
<div id="repeat" class="probe repeat"></div>
<div id="oversized_literal" class="probe oversized" style="padding: 1e50px !important"></div>
<div id="oversized_variable" class="probe oversized"
     style="--oversized: 1e50px; padding: var(--oversized) !important"></div>
<div id="background_layers" class="probe"></div>
<div id="var_specific" class="probe variable" style="width: var(--Width)"></div>
<div id="var_important" class="probe var_important" style="width: var(--Width); --Width: 1px"></div>
<div class="inherit_var"><div id="inherit_var" class="probe" style="--Width: 1px; --Width: revert; width: var(--Width)"></div></div>
<div id="shorthand" class="probe shorthand" style="--border: 2px solid black; border-right-width: 5px !important"></div>
<div id="invalid_var" class="probe invalid_var" style="--bad: bogus"></div>
<div id="ignored_var" class="probe ignored_var" style="width: 41px; --bad: bogus"></div>
<div id="unset_rollback" class="probe" style="width: revert"></div>
<div id="unlayered_rollback" class="probe" style="width: revert-layer !important"></div>
<div id="unlayered_var_rollback" class="probe" style="width: var(--missing, revert-layer) !important"></div>
<div class="inherit_var">
  <div id="unlayered_custom" class="probe" style="--Width: revert-layer; width: var(--Width)"></div>
</div>
<i id="pseudo_weight" class="pseudo_weight">text</i>
<div id="is_case" class="probe is_case extra"></div>
<div id="not_case" class="probe not_case extra"></div>
<section><div id="nth_case" class="probe nth_case extra"></div></section>
<section><div id="nth_last_case" class="probe nth_last_case extra"></div></section>
<div id="pseudo" class="pseudo" style='--after: "AFTER"'>middle</div>
<div id="pseudo_tie">suffix</div>
<div id="pseudo_reset">host text</div>
<div id="pseudo_host"><span id="pseudo_child">node</span></div>
<div id="pseudo_sibling" style="height: 4px"></div>
<div style="--base: 60px">
  <div id="custom_rollback" class="probe"
       style="--base: revert; --result: calc(var(--base) + 2px); width: var(--result)"></div>
  <div id="custom_var_rollback" class="probe"
       style="--base: var(--missing, revert); --result: calc(var(--base) + 3px); width: var(--result)"></div>
  <div id="custom_var_inherit" class="probe"
       style="--base: var(--missing, inherit); width: var(--base, 56px)"></div>
  <div id="custom_var_unset" class="probe"
       style="--base: var(--missing, unset); width: var(--base, 56px)"></div>
  <div id="custom_var_initial" class="probe"
       style="--base: var(--missing, initial); width: var(--base, 56px)"></div>
</div>
<div id="custom_var_no_baseline" class="probe" style="--base: var(--missing, revert-layer); width: var(--base, 64px)"></div>
<div id="valid_box" class="probe computed_box" style="--padding: 1px 2px"></div>
<div id="invalid_box" class="probe computed_box" style="--padding: 1px bogus; padding-left: 7px"></div>
<div id="rollback_box" class="probe computed_box" style="--undo: revert; padding: var(--undo); padding-left: 7px"></div>
<p id="aligned" align="right" style="text-align: var(--missing, revert)"><span id="aligned_text">align</span></p>
<p align="right"><span id="reference_text">align</span></p>
<p class="baseline_var" align="var(--alignment)" style="--alignment: right; text-align: var(--missing, revert-layer)">
  <span id="baseline_var_text">align</span>
</p>
</body></html>
"""

const ADDITIONAL: String = """
@import "styles/additional.css";
.probe.specific { width: 12px; }
.tie { width: 33px; }
.important_specific { width: 12px !important; }
.normal_specific { width: 40px !important; }
.variable { --Width: 1px; }
.nested_deferred { width: 61px; }
.is_case.extra, .not_case.extra, .nth_case.extra, .nth_last_case.extra { width: 12px; }
.pseudo::before { content: "OVERRIDDEN"; }
#pseudo::after { content: var(--after); }
#pseudo_tie::before { content: var(--prefix, "ADDITIONAL"); }
#pseudo_reset::before, #pseudo_reset::after { content: var(--rollback, revert-layer) !important; }
@media (min-width: 400px) and (max-width: 450px) { .link_media { width: 40px; } }
#pseudo_child::before { content: "WRONG_CHILD"; }
#pseudo_sibling { width: 63px; }
.unused { width: bogus; }
"""

const FEATURE_QUERIES: String = """
<!doctype html><style>
@import "styles/conditional.css" supports(display: grid) (min-width: 300px);
@import "styles/rejected.css" supports(display: unsupported);
@import "styles/conditional.css" supports((color: color-mix(red, blue))) (max-width: 299px);
body { margin: 0; }
@supports (display: grid) and ((width: calc(5px + 5px)) or (unknown: yes)) {
  #conditional { width: 41px; }
  @media (min-width: 400px) {
    @supports (color: var(--color) !important) { #conditional { width: 43px; } }
  }
}
@supports not (display: unsupported) { #negated { width: 42px; } }
@supports (display: grid)and/**/(width: 2px) { #compact { width: 50px; } }
@supports (display: grid) and (width: 2px) or (display: grid) { #negated { width: 99px; } }
@supports not future-query() { #future { width: 44px; } }
@supports (width: 10px; height: 20px) { #future { width: 99px; } }
@supports (text-emphasis-style: dot) { #emphasis_support { width: 53px; } }
@supports (border-top-style: groove) { #bevel_support { width: 52px; } }
@supports selector(.parent:has(> .leaf)) { #selector_support { width: 45px; } }
@supports selector(:is(.supported, :unsupported-example)) { #selector_support { width: 99px; } }
@supports selector(.supported, .other) { #selector_support { width: 99px; } }
@supports selector(.related:has(> :is(.leaf, q|div))) { #selector_support { width: 99px; } }
#specificity { width: 12px; }
:where(#specificity) { width: 99px; }
.related:has(> .leaf, #absent) { width: 46px; }
.related.special { width: 99px; }
.sibling:has(+ .next > .leaf) { width: 47px; }
.sibling:has(~ .later .leaf) { height: 48px; }
.related:has(.leaf:has(span)) { width: 99px; }
.related:has(::before) { width: 99px; }
.related:has(> .leaf)::before { content: "RELATIVE"; }
.empty:empty { width: 49px; }
.nonempty:empty { width: 99px; }
</style>
<div id="conditional"></div><div id="negated"></div><div id="future"></div><div id="compact"></div>
<div id="selector_support"></div><div id="emphasis_support"></div><div id="conditional_import"></div><div id="specificity"></div><div id="bevel_support"></div>
<div id="related" class="related special"><span class="leaf"></span></div>
<div id="sibling" class="sibling"></div><!-- Adjacent selectors skip DOM comments -->
<div class="next"><span class="leaf"></span></div><div></div><div class="later"><span class="leaf"></span></div>
<div id="empty" class="empty"> <!-- No content --> </div><div class="nonempty" id="nonempty"><span></span></div>
"""

const LAYERS: String = """
<!doctype html><style>
@layer base, theme;
@import "styles/layer.css" layer(base.widgets) supports(display: grid);
@import "styles/anonymous-layer.css" layer;
@import "styles/layer-media.css" layer(media_first) (min-width: 400px);
@import "styles/rejected.css" layer(rejected) supports(display: unsupported);
@layer after_import;
@import "styles/conditional.css" layer(after_import);
@layer default { #default_layer { width: 72px; } }
body { margin: 0; }
@media (min-width: 400px) { @layer breakpoint; }
@layer stable, breakpoint;
@layer optional, media_first;
@layer theme {
  .normal { width: 51px; }
  #important, #inline_layer_important { width: 91px !important; }
  #rollback { width: var(--undo, revert-layer); }
  #rollback_var { --size: revert-layer; }
  #rollback_var_deferred, #rollback_var_cycle { --size: var(--undo, revert-layer); }
  #rollback_chain { width: var(--undo, revert-layer); }
  #layer_shorthand { margin: var(--undo, revert-layer); margin-right: 7px; }
  #layer_pseudo::before { content: "THEME"; }
  #bad_layer_value { width: var(--missing); }
}
@layer base {
  #normal, #outside, #inline_layer_normal { width: 92px; }
  .important, .inline_layer_important { width: 52px !important; }
  #rollback { width: var(--size, 53px); }
  #rollback_var { --size: 54px; }
  #rollback_var_deferred { --size: calc(var(--source) + 2px); }
  #rollback_var_cycle { --size: var(--next, 98px); --next: var(--size, 99px); }
  #rollback_chain { width: 55px; }
  #important_rollback { width: revert-layer !important; }
  #layer_shorthand { margin: 3px 4px 5px 6px; }
  #layer_pseudo::before { content: "BASE"; }
  #bad_layer_value { width: 93px; }
  .widgets_parent { width: 56px; }
}
@layer theme.deep {
  #rollback_chain { width: revert-layer; }
}
@layer base.widgets { #widgets_parent { width: 95px; } }
@layer { #anonymous { width: 57px; } }
@layer { .anonymous { width: 58px; } }
@layer stable { #media_order { width: 59px; } }
@layer breakpoint { #media_order { width: 60px; } }
@layer optional { #import_media { width: 61px; } }
@layer media_first { #import_media { width: 62px; } }
@layer malformed..name { #normal { width: 99px !important; } }
@layer Case { #case_name { width: 63px; } }
@layer case { .case_name { width: 64px; } }
#outside { width: 65px; }
#conditional_import { width: revert-layer; }
#important { width: 99px !important; }
#rollback_var { width: var(--size); }
#rollback_var_deferred { --source: 70px; width: var(--size); }
#rollback_var_cycle { width: var(--size, 73px); }
#inline_rollback, #inline_rollback_important { width: 66px; }
#inline_rollback_important { width: 67px !important; }
#layer_shorthand { width: 10px; }
</style>
<div id="normal" class="normal"></div>
<div id="important" class="important"></div>
<div id="outside"></div>
<div id="inline_layer_normal" style="width: 68px"></div>
<div id="inline_layer_important" class="inline_layer_important" style="width: 69px !important"></div>
<div id="rollback"></div>
<div id="rollback_var"></div>
<div id="rollback_var_deferred"></div>
<div id="rollback_var_cycle"></div>
<div id="rollback_chain"></div>
<div id="important_rollback"></div>
<div id="layer_shorthand"></div>
<div id="layer_pseudo">content</div>
<div id="bad_layer_value"></div>
<div id="widgets_parent" class="widgets_parent"></div>
<div id="layer_import"></div>
<div id="nested_layer_import"></div>
<div id="conditional_import"></div>
<div id="default_layer"></div>
<div id="anonymous" class="anonymous"></div>
<div id="media_order"></div>
<div id="import_media"></div>
<div id="case_name" class="case_name"></div>
<div id="inline_rollback" style="width: revert-layer"></div>
<div id="inline_rollback_important" style="width: revert-layer !important"></div>
"""

const ADDITIONAL_LAYERS: String = """
@layer base.widgets {
  #layer_import { width: 95px; }
  @layer components { #nested_layer_import { width: 73px; } }
}
@layer older {
  #important_rollback { width: 98px !important; }
}
@layer theme {
  .normal { width: 70px; }
  #layer_pseudo::after { content: "ADDITIONAL"; }
}
"""


var _failures: int


func _initialize() -> void:
	call_deferred("_run")


func _run() -> void:
	_check_layout()
	_check_inline_reflow()
	_check_css_features()
	_check_selector_semantics()
	_check_layers()
	_check_box_and_transform_functions()
	_check_math_functions()
	_check_length_units()
	_check_grid_intrinsic_tracks()
	_check_intrinsic_box_widths()
	_check_aspect_ratio()
	await _check_object_sizing()
	await _check_image_rendering()
	await _check_image_set()
	await _check_annotated_image()
	await _check_cross_fade()
	await _check_image_resolution()
	await _check_image_orientation()
	await _check_border_image()
	await _check_outlines()
	await _check_background_tiling()
	await _check_image_positions()
	await _check_background_attachment()
	await _check_embedded_background_attachment()
	_check_generated_content()
	await _check_generated_image()
	await _check_list_markers()
	_check_counter_styles()
	await _check_font_faces()
	await _check_oblique_angles()
	_check_font_width()
	_check_opentype_settings()
	_check_font_variants()
	_check_named_font_alternates()
	_check_historical_forms()
	_check_east_asian_variants()
	_check_optical_sizing()
	await _check_text_spacing()
	_check_text_autospace()
	_check_punctuation_spacing()
	await _check_text_transform()
	await _check_line_alignment()
	await _check_text_indent_and_tabs()
	await _check_white_space()
	await _check_white_space_trim()
	_check_text_wrap_styles()
	_check_cjk_word_break()
	_check_line_break_strictness()
	await _check_line_break_anywhere()
	_check_overflow_wrap()
	_check_word_break_all()
	_check_lexical_word_breaks()
	await _check_discretionary_hyphenation()
	_check_hyphenation_limits()
	_check_hyphenation_character_limits()
	_check_hyphenation_last_line()
	await _check_automatic_hyphenation()
	await _check_controls()
	await _check_author_hover()
	await _check_focus_within()
	await _check_text_emphasis()
	await _check_underline_position()
	await _check_color_spaces()
	print("CSS cascade checks finished with %d failure(s)" % _failures)
	quit(0 if _failures == 0 else 1)


func _check_layout() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	if layout.parse(DOCUMENT, "res://", ADDITIONAL) != OK:
		_check(false, "Parse cascade fixture: " + layout.get_render_error())
		return
	_check(layout.layout(Vector2i(320, 500)) == OK, "Lay out the narrow viewport")
	var widths: Dictionary[String, float] = {
		"from_link": 30.0,
		"style_type": 66.0,
		"order": 31.0,
		"import_order": 23.0,
		"invalid_nested": 320.0,
		"after_link": 34.0,
		"reimport_order": 24.0,
		"stylesheet_move": 31.0,
		"link_type": 10.0,
		"link_media": 12.0,
		"specific": 32.0,
		"tie": 33.0,
		"inline_normal": 35.0,
		"rule_important": 36.0,
		"inline_important": 37.0,
		"spaced_important": 54.0,
		"spaced_inline": 56.0,
		"invalid_deferred": 55.0,
		"unused_fallback": 57.0,
		"spaced_shorthand": 22.0,
		"inline_important_vs_normal": 39.0,
		"important_specific": 38.0,
		"normal_specific": 40.0,
		"repeat": 42.0,
		"oversized_literal": 31.0,
		"oversized_variable": 25.0,
		"background_layers": 53.0,
		"var_specific": 43.0,
		"var_important": 44.0,
		"inherit_var": 45.0,
		"shorthand": 27.0,
		"invalid_var": 320.0,
		"ignored_var": 41.0,
		"unset_rollback": 320.0,
		"unlayered_rollback": 320.0,
		"unlayered_var_rollback": 320.0,
		"unlayered_custom": 45.0,
		"is_case": 46.0,
		"not_case": 47.0,
		"nth_case": 48.0,
		"nth_last_case": 49.0,
		"custom_rollback": 62.0,
		"custom_var_rollback": 63.0,
		"custom_var_inherit": 60.0,
		"custom_var_unset": 60.0,
		"custom_var_initial": 56.0,
		"custom_var_no_baseline": 64.0,
		"valid_box": 34.0,
		"invalid_box": 32.0,
		"rollback_box": 32.0,
		"pseudo_sibling": 63.0,
	}
	for id: String in widths:
		var actual: float = _rect(layout, id).size.x
		_check(is_equal_approx(actual, widths[id]), "%s width is %s, expected %s" % [id, actual, widths[id]])
	var rendered: String = layout.get_plain_text()
	_check("SPECIFICmiddleIMPORTANT" in rendered and "OVERRIDDEN" not in rendered,
		"Pseudo-elements compare importance before specificity")
	_check("ADDITIONALsuffix" in rendered, "Additional pseudo-element rule wins a source-order tie")
	_check("host text" in rendered and "REMOVED_GENERATED_TEXT" not in rendered,
		"A winning variable rollback clears author content on both generated pseudo-elements")
	_check("PSEUDOtextTWO" in rendered, "Legacy and double-colon pseudo-elements use the same specificity")
	_check("HOSTCHILDnode" in rendered and "INVALID_" not in rendered,
		"Only the selector's supported terminal pseudo-element receives the declaration")
	var alignment: float = _rect(layout, "aligned_text").position.x
	_check(alignment > 100.0 and is_equal_approx(alignment, _rect(layout, "reference_text").position.x),
		"Variable revert restores the HTML align hint")
	_check(is_equal_approx(_rect(layout, "baseline_var_text").position.x, alignment),
		"The restored baseline declaration resolves custom properties")
	_check(_has_diagnostic(layout, "width", "bogus", "style_element"), "Invalid literal reported despite losing")
	_check(_has_diagnostic(layout, "width", "var(--bad)", "style_element"), "Invalid computed value retains its tokens")
	_check(layout.get_element_by_id("ignored_var").get_css_diagnostics().is_empty(),
		"A losing variable declaration is not evaluated")
	_check(_has_diagnostic(layout, "width", "bogus", "local_stylesheet", "res://styles/nested/inner.css"),
		"Nested import diagnostics retain the stylesheet path")
	_check(_has_diagnostic(layout, "width", "var(--nested-width)", "local_stylesheet", "res://styles/nested/inner.css"),
		"A winning variable declaration retains its nested import path after substitution fails")
	_check(_has_diagnostic(layout, "width", "bogus", "stylesheet"), "Additional CSS diagnostics retain their source")
	_check(_has_diagnostic(layout, "padding", "var(--padding)", "style_element"),
		"An invalid computed shorthand reports the original declaration")
	_check(_has_diagnostic(layout, "padding", "var(--oversized)", "inline_style"),
		"An out-of-range length in a winning shorthand reports its original variable expression")
	_check(not _has_diagnostic(layout, "width", "bogus", "local_stylesheet", "res://styles/rejected.css"),
		"A rel token containing stylesheet as a substring does not load CSS")

	var original: HTMLElement = layout.get_element_by_id("specific")
	_check(layout.layout(Vector2i(420, 500)) == OK and is_equal_approx(_rect(layout, "link_media").size.x, 40.0),
		"The additional stylesheet wins a source-order tie while both media conditions match")
	_check(layout.layout(Vector2i(480, 500)) == OK, "Reflow across the media breakpoint")
	_check(is_equal_approx(original.get_bounding_rect().size.x, 50.0), "Matched media rules use the same cascade")
	_check(is_equal_approx(_rect(layout, "link_media").size.x, 29.0), "Linked stylesheet media condition")
	_check(original.set_attribute("id", "changed") == OK, "Update an element ID")
	_check(layout.layout(Vector2i(480, 500)) == OK and original.is_valid(), "Reflow after a DOM mutation")
	_check(is_equal_approx(original.get_bounding_rect().size.x, 12.0), "DOM mutation recomputes the rule winner")
	var link: HTMLElement = layout.get_element_by_id("main_sheet")
	_check(link.set_attribute("rel", "next") == OK, "Remove the stylesheet relationship")
	_check(layout.layout(Vector2i(480, 500)) == OK and is_equal_approx(_rect(layout, "from_link").size.x, 12.0),
		"Recomputing the cascade removes rules from detached stylesheet relationships")
	_check(link.set_attribute("rel", "next STYLESHEET") == OK, "Restore the stylesheet relationship")
	_check(layout.layout(Vector2i(480, 500)) == OK and is_equal_approx(_rect(layout, "from_link").size.x, 30.0),
		"Recomputing the cascade restores linked rules in document order")
	_check(link.set_attribute("disabled", "") == OK, "Disable the linked stylesheet")
	_check(layout.layout(Vector2i(480, 500)) == OK and is_equal_approx(_rect(layout, "from_link").size.x, 12.0),
		"Disabling a linked sheet removes its declarations from the cascade")
	_check(link.remove_attribute("disabled") == OK, "Enable the linked stylesheet")
	_check(layout.layout(Vector2i(480, 500)) == OK and is_equal_approx(_rect(layout, "from_link").size.x, 30.0),
		"Enabling a linked sheet restores its source order")
	var style: HTMLElement = layout.get_element_by_id("inactive_style")
	_check(style.set_attribute("type", "") == OK, "Enable an embedded stylesheet with an empty CSS type")
	_check(layout.layout(Vector2i(480, 500)) == OK and is_equal_approx(_rect(layout, "style_type").size.x, 67.0),
		"The newly enabled stylesheet participates in the author cascade")
	_check(style.set_attribute("type", "text/plain") == OK, "Change the embedded stylesheet to a non-CSS type")
	_check(layout.layout(Vector2i(480, 500)) == OK and is_equal_approx(_rect(layout, "style_type").size.x, 66.0),
		"Changing the stylesheet type removes its previous winning declarations")
	var typed_link: HTMLElement = layout.get_element_by_id("typed_sheet")
	_check(not _has_diagnostic(layout, "width", "bogus", "local_stylesheet", "res://styles/typed.css"),
		"An unsupported link type contributes no CSS diagnostics")
	_check(typed_link.set_attribute("type", "TeXt/CsS; charset=utf-8") == OK, "Enable the linked CSS type")
	_check(layout.layout(Vector2i(480, 500)) == OK and is_equal_approx(_rect(layout, "link_type").size.x, 68.0),
		"A link that gains a supported MIME type contributes its author declarations")
	_check(_has_diagnostic(layout, "width", "bogus", "local_stylesheet", "res://styles/typed.css"),
		"An enabled link contributes diagnostics with its resource path")
	_check(typed_link.set_attribute("type", "text/plain") == OK, "Restore the unsupported linked stylesheet type")
	_check(layout.layout(Vector2i(480, 500)) == OK and is_equal_approx(_rect(layout, "link_type").size.x, 10.0),
		"A link that loses its supported MIME type stops contributing author declarations")
	_check(not _has_diagnostic(layout, "width", "bogus", "local_stylesheet", "res://styles/typed.css"),
		"Changing the link type also removes stale diagnostics")
	var leading_style: HTMLElement = layout.get_element_by_id("leading_styles")
	_check(leading_style.get_parent().append_child(leading_style) == OK,
		"Move the leading style element after the linked stylesheet")
	_check(layout.layout(Vector2i(480, 500)) == OK and is_equal_approx(_rect(layout, "stylesheet_move").size.x, 21.0),
		"Moving the style element changes the source-order winner")


func _check_inline_reflow() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var source: String = """
		<style>
			body { margin: 0; }
			#probe { width: 40px !important; height: 8px; }
		</style>
		<div id="probe" style="--scale: 25vw; width: var(--scale) !important; height: nonsense"></div>
	"""
	if layout.parse(source) != OK or layout.layout(Vector2i(200, 100)) != OK:
		_check(false, "Prepare inline cascade reflow fixture: " + layout.get_render_error())
		return
	var probe: HTMLElement = layout.get_element_by_id("probe")
	_check(probe.get_bounding_rect().size.is_equal_approx(Vector2(50.0, 8.0)),
		"The inline important variable wins while the invalid inline height leaves the author height")
	_check(probe.get_css_diagnostics().size() == 1 and _has_diagnostic(layout, "height", "nonsense", "inline_style"),
		"The invalid inline declaration retains its source")
	_check(layout.layout(Vector2i(240, 100)) == OK, "Resize with the same author rules")
	_check(probe.get_bounding_rect().size.is_equal_approx(Vector2(60.0, 8.0)),
		"A viewport-only reflow updates the winning inline length")
	_check(probe.get_css_diagnostics().size() == 1, "A viewport-only reflow retains one inline diagnostic")
	_check(probe.set_attribute("style", "width: 90px; height: 12px") == OK, "Replace inline declarations")
	_check(layout.layout(Vector2i(240, 100)) == OK, "Reflow after replacing inline declarations")
	_check(probe.get_bounding_rect().size.is_equal_approx(Vector2(40.0, 12.0)),
		"Editing inline style withdraws its previous important winner")
	_check(probe.get_css_diagnostics().is_empty(), "Editing inline style clears stale diagnostics")
	_check(probe.remove_attribute("style") == OK and layout.layout(Vector2i(240, 100)) == OK,
		"Remove the inline declaration block and reflow")
	_check(probe.get_bounding_rect().size.is_equal_approx(Vector2(40.0, 8.0)),
		"Removing inline style restores the stylesheet winners")


func _check_css_features() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	_check(layout.parse(FEATURE_QUERIES) == OK and layout.layout(Vector2i(320, 480)) == OK,
		"Lay out the selector and feature-query fixture")
	var widths: Dictionary[String, float] = {
		"conditional": 41.0,
		"negated": 42.0,
		"future": 44.0,
		"compact": 50.0,
		"selector_support": 45.0,
		"conditional_import": 51.0,
		"specificity": 12.0,
		"related": 46.0,
		"sibling": 47.0,
		"empty": 49.0,
		"bevel_support": 52.0,
		"emphasis_support": 53.0,
		"nonempty": 320.0,
	}
	for id: String in widths:
		_check(is_equal_approx(_rect(layout, id).size.x, widths[id]), "Feature query or selector width for %s" % id)
	_check(is_equal_approx(_rect(layout, "sibling").size.y, 48.0), "General sibling and descendant relative selector")
	_check(layout.query_selector("div:has(> .leaf)") == layout.get_element_by_id("related"), "Query with :has()")
	_check(layout.query_selector("div:has(> :is(.leaf, :unsupported-example))") == layout.get_element_by_id("related"),
		"An inner :is() remains forgiving during normal matching")
	_check(layout.query_selector_all(":where(.next, .later, :unsupported-example)").size() == 2,
		":where() parses forgiving selector lists")
	_check("RELATIVE" in layout.get_plain_text(), "A relational selector styles a pseudo-element")
	_check(layout.get_css_diagnostics().is_empty(), "Feature probes and rejected imports leave diagnostics empty")
	_check(layout.layout(Vector2i(480, 480)) == OK, "Lay out nested media and feature queries")
	_check(is_equal_approx(_rect(layout, "conditional").size.x, 43.0), "Nested @media and @supports")
	_check(layout.layout(Vector2i(280, 480)) == OK, "Lay out the alternate conditional import")
	_check(is_equal_approx(_rect(layout, "conditional_import").size.x, 51.0), "Parenthesized import supports condition")
	var leaf: HTMLElement = layout.query_selector(".related > .leaf")
	_check(leaf.set_attribute("class", "other") == OK and layout.layout(Vector2i(280, 480)) == OK,
		"Mutate a relative selector's matched child")
	_check(not layout.get_element_by_id("related").matches(":has(> .leaf)"), "Relative matching follows DOM changes")
	_check(is_equal_approx(_rect(layout, "related").size.x, 99.0), "Relational styles update on mutation")
	_check("RELATIVE" not in layout.get_plain_text(), "Generated content disappears with its relative match")


func _check_selector_semantics() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	var html: String = """<!doctype html><html lang="en-US"><head><style>
	body { margin: 0; }
	#list > div, #languages > div { height: 5px; }
	:scope > body #scope_style { width: 31px; }
	#latin:lang(EN, "*-Hant") { width: 32px; }
	#traditional:lang("*-HANT-TW") { width: 33px; }
	#extension:lang("*-Hant") { width: 99px; }
	#unknown:lang("") { width: 34px; }
	#unknown:lang("*") { width: 99px; }
	#custom:lang(xyzzy) { width: 35px; }
	#list > :nth-child(2n+1 of .pick) { width: 36px; }
	#list > :nth-last-child(2 of .pick, .other) { height: 38px; }
	#list > :nth-child(1 of .pick, :unknown-example) { width: 99px; }
	#list > :nth-child(1 of) { width: 99px; }
	#list > :nth-child(2n 1) { width: 99px; }
	#list > :nth-child(2147483647n+1) { height: 39px; }
	@supports selector(:scope > :lang(en, "*-Hant")) { #feature { width: 40px; } }
	@supports selector(:lang(en fr)) { #feature { width: 99px; } }
	@supports selector(:nth-child(2.5n + 1)) { #feature { width: 99px; } }
	@supports selector(:nth-child(1 of .pick, :unknown-example)) { #feature { width: 99px; } }
	</style></head><body>
	<div id="scope_style"></div>
	<div id="languages">
		<div id="latin"></div>
		<div id="traditional" lang="zh-Hant-TW"></div>
		<div id="extension" lang="zh-x-Hant"></div>
		<div id="unknown" lang=""><span id="inherits_unknown">unknown</span></div>
		<div id="custom" lang="xyzzy"></div>
	</div>
	<div id="list"><div id="first" class="pick"></div><!-- skip -->
		<div id="second" class="other"></div><div id="third" class="pick"></div>
		<div id="fourth" class="pick" style="display: none"></div>
	</div>
	<div id="feature"></div>
	<a id="link" href="https://example.test/">link</a><a id="no_href">text</a>
	<map name="map"><area id="area" href="#scope_style"></map>
	</body></html>"""
	_check(layout.parse(html) == OK and layout.layout(Vector2i(220, 380)) == OK, "Lay out selector semantics")
	var widths: Dictionary[String, float] = {
		"scope_style": 31.0,
		"latin": 32.0,
		"traditional": 33.0,
		"extension": 220.0,
		"unknown": 34.0,
		"custom": 35.0,
		"first": 36.0,
		"third": 220.0,
		"feature": 40.0,
	}
	for id: String in widths:
		var width: float = _rect(layout, id).size.x
		_check(is_equal_approx(width, widths[id]), "Selector width for %s is %s, expected %s" % [id, width, widths[id]])
	_check(is_equal_approx(_rect(layout, "first").size.y, 39.0), "Large An+B coefficients do not overflow")
	_check(is_equal_approx(_rect(layout, "third").size.y, 38.0), "nth-last-child counts matching hidden siblings")
	var root_element: HTMLElement = layout.query_selector(":scope")
	var languages: HTMLElement = layout.get_element_by_id("languages")
	var latin: HTMLElement = layout.get_element_by_id("latin")
	var unknown: HTMLElement = layout.get_element_by_id("inherits_unknown")
	_check(root_element.matches("html:root:first-child:last-of-type:nth-child(1 of html)"), "Structural selectors match the root")
	_check(languages.query_selector(":scope > :lang(en-us)") == latin, "Element query uses its own scope")
	_check(languages.query_selector("html:scope #latin") == null, "Element query has exactly one scope root")
	_check(latin.closest(":scope") == latin and layout.query_selector(":scope") == root_element, "Queries restore the previous scope")
	var detached: HTMLElement = layout.create_element("section")
	var detached_child: HTMLElement = layout.create_element("span")
	_check(detached.append_child(detached_child) == OK, "Build a detached subtree")
	_check(detached.matches(":scope:only-child") and detached.query_selector(":scope > span") == detached_child,
		"Detached nodes use the explicit query scope")
	_check(languages.append_child(detached) == OK and detached_child.closest("#languages :scope") == detached_child,
		"Moving an element preserves the handle and lets closest inspect ancestors outside the scope")
	_check(unknown.matches(':lang("")') and not unknown.matches(':lang("*")'), "An empty language stops inheritance")
	_check(root_element.matches(r":lang(\65 n)"), "Escapes in language ranges are decoded by the tokenizer")
	_check(layout.query_selector(r"#list > :nth-child(2\6e +1)") == layout.get_element_by_id("first"), "An+B accepts escaped units")
	_check(layout.query_selector("#list > :nth-child(+n - 1 of .pick)") == layout.get_element_by_id("first"), "An+B accepts adjacent plus")
	_check(layout.query_selector_all(":any-link").size() == 2, "Hyperlink pseudo-class covers anchors and areas")
	_check(layout.get_element_by_id("area").matches(":link") and not layout.get_element_by_id("no_href").matches(":link"),
		"Link pseudo-class requires href")
	var link: HTMLElement = layout.get_element_by_id("link")
	_check(link.remove_attribute("href") == OK and not link.matches(":any-link"), "Removing href updates queries immediately")
	_check(languages.set_attribute("lang", "ja-JP") == OK and latin.matches(":lang(ja)"), "Language mutation is inherited immediately")
	_check(layout.layout(Vector2i(220, 380)) == OK and is_equal_approx(_rect(layout, "latin").size.x, 220.0),
		"Language mutation also invalidates the stylesheet match")
	var quirks: HTMLLayout = HTMLLayout.new()
	_check(quirks.parse('<div id="Mixed" class="Case" data-empty=""></div>') == OK, "Parse selector attributes in quirks mode")
	var mixed: HTMLElement = quirks.query_selector("#mixed.case")
	_check(mixed != null and mixed.matches('[id="Mixed"][class="Case"][data-empty=""]'),
		"Quirks selectors fold ID and class while preserving raw attribute values")
	_check(mixed != null and not mixed.matches('[id="mixed"], [class="case"], [data-missing=""]'),
		"Attribute matching distinguishes original case, missing attributes and empty values")


func _check_layers() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	_check(layout.parse(LAYERS, "res://", ADDITIONAL_LAYERS) == OK and layout.layout(Vector2i(320, 500)) == OK,
		"Parse and lay out cascade layers")
	var widths: Dictionary[String, float] = {
		"normal": 70.0,
		"important": 52.0,
		"outside": 65.0,
		"inline_layer_normal": 68.0,
		"inline_layer_important": 69.0,
		"rollback": 53.0,
		"rollback_var": 54.0,
		"rollback_var_deferred": 72.0,
		"rollback_var_cycle": 73.0,
		"rollback_chain": 55.0,
		"important_rollback": 320.0,
		"bad_layer_value": 320.0,
		"widgets_parent": 56.0,
		"layer_import": 71.0,
		"nested_layer_import": 73.0,
		"conditional_import": 320.0,
		"default_layer": 72.0,
		"anonymous": 58.0,
		"media_order": 60.0,
		"import_media": 62.0,
		"case_name": 64.0,
		"inline_rollback": 66.0,
		"inline_rollback_important": 67.0,
	}
	for id: String in widths:
		var width: float = _rect(layout, id).size.x
		_check(is_equal_approx(width, widths[id]), "Layer width for %s is %s, expected %s" % [id, width, widths[id]])
	_check(is_equal_approx(_rect(layout, "layer_shorthand").position.x, 6.0),
		"Deferred shorthand rolls back only its winning longhands")
	_check("THEMEcontentADDITIONAL" in layout.get_plain_text(), "Generated pseudo-elements use cascade layers")
	_check(_has_diagnostic(layout, "width", "var(--missing)", "style_element"),
		"An invalid winning substitution in a layer retains the declaration diagnostic")
	_check(_has_diagnostic(layout, "width", "bogus", "local_stylesheet", "res://styles/nested/layer-inner.css"),
		"Nested layered imports retain their diagnostic paths")
	_check(not _has_diagnostic(layout, "width", "bogus", "local_stylesheet", "res://styles/rejected.css"),
		"A rejected layered import leaves no diagnostics")
	_check(layout.layout(Vector2i(480, 500)) == OK, "Reflow cascade layers across a media breakpoint")
	_check(is_equal_approx(_rect(layout, "media_order").size.x, 59.0), "A media condition changes layer registration order")
	_check(is_equal_approx(_rect(layout, "import_media").size.x, 61.0),
		"A media condition activates a layered import at its source position")
	_check(layout.layout(Vector2i(320, 500)) == OK, "Deactivate media layers")
	_check(is_equal_approx(_rect(layout, "media_order").size.x, 60.0), "Media layers can be removed from the ordering")


func _check_box_and_transform_functions() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	var html: String = """
<!doctype html><style>
body { margin: 0; }
div { position: absolute; width: 20px; height: 10px; transform-origin: 0 0; }
#inset { --offsets: 5px auto auto calc(2px + 3px); inset: var(--offsets); }
#matrix { inset: 10px auto auto 50px; transform: matrix(2, 0, 0, 3, 4, 6); }
#skew_x { inset: 60px auto auto 20px; transform: skewX(45deg); }
#skew_y { inset: 80px auto auto 20px; transform: skewY(45deg); }
#skew { inset: 100px auto auto 20px; transform: skew(45deg, 45deg); }
#combined { inset: 145px auto auto 20px; transform: translateX(5px) skew(45deg, 0); }
#invalid { inset: 170px auto auto 20px; transform: translateX(5px); transform: matrix(1,0,0,1,5px,0); }
</style>
<div id="inset" style="left: 15px; inset: 1px bad"></div>
<div id="matrix"></div><div id="skew_x"></div><div id="skew_y"></div><div id="skew"></div>
<div id="combined"></div><div id="invalid"></div>
"""
	_check(layout.parse(html) == OK and layout.layout(Vector2i(250, 240)) == OK, "Lay out affine and inset fixture")
	var rectangles: Dictionary[String, Rect2] = {
		"inset": Rect2(15.0, 5.0, 20.0, 10.0),
		"matrix": Rect2(54.0, 16.0, 40.0, 30.0),
		"skew_x": Rect2(20.0, 60.0, 30.0, 10.0),
		"skew_y": Rect2(20.0, 80.0, 20.0, 30.0),
		"skew": Rect2(20.0, 100.0, 30.0, 30.0),
		"combined": Rect2(25.0, 145.0, 30.0, 10.0),
		"invalid": Rect2(25.0, 170.0, 20.0, 10.0),
	}
	for id: String in rectangles:
		var actual: Rect2 = _rect(layout, id)
		_check(actual.is_equal_approx(rectangles[id]), "%s bounds are %s, expected %s" % [id, actual, rectangles[id]])
	_check(layout.get_element_at(Vector2(22.0, 68.0)) != layout.get_element_by_id("skew_x"),
		"Hit testing skips the skewed rectangle's empty corner")
	_check(layout.get_element_at(Vector2(36.0, 68.0)) == layout.get_element_by_id("skew_x"),
		"Hit testing finds the skewed rectangle")


func _check_math_functions() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var widths: Dictionary[String, float] = {
		"round(35px, 10px)": 40.0,
		"round(up, 31px, 10px)": 40.0,
		"round(down, 39px, 10px)": 30.0,
		"calc(50px + round(to-zero, -19px, 10px))": 40.0,
		"calc(round(-1.5) * -10px)": 10.0,
		"mod(-13px, 5px)": 2.0,
		"calc(20px + rem(-13px, 5px))": 17.0,
		"calc(20px + mod(13px, -5px))": 18.0,
		"calc(40px * sin(pi / 6))": 20.0,
		"calc(40px * cos(60deg))": 20.0,
		"calc(20px * tan(50grad))": 20.0,
		"calc(30px * sin(asin(1)))": 30.0,
		"calc(30px * sin(acos(0)))": 30.0,
		"calc(25px * sin(atan(1) + 45deg))": 25.0,
		"calc(20px * sin(atan2(1in, 96px) + 45deg))": 20.0,
		"calc(2px * pow(3, 2) + sqrt(16) * 1px)": 22.0,
		"hypot(3px, 4px, 12px)": 13.0,
		"calc((log(100, 10) + log(e) + exp(0)) * 5px)": 20.0,
		"calc(abs(-25px) + sign(-2em) * 5px)": 20.0,
		"calc(10px * (2in / 96px))": 20.0,
		"calc(10px * (1em / 1px))": 120.0,
		"calc(10px * (180deg / 0.5turn))": 10.0,
		"min(44px, calc(infinity * 1px))": 44.0,
		"calc(20px * max(0, sign(-0)))": 0.0,
		"calc(20px + rem(2px, 0px))": 0.0,
		"calc(20px + pow(NaN, 0) * 1px)": 0.0,
		"calc(20px + min(3, hypot(infinity, NaN)) * 1px)": 0.0,
		"round(23px)": 19.0,
		"sin(10px)": 19.0,
		"pow(2px, 2)": 19.0,
		"hypot(3px, 4deg)": 19.0,
	}
	var html: String = """<!doctype html><style>
	body { margin: 0; }
	.probe { height: 4px; font-size: 12px; }
	#responsive { width: round(down, 37%, 10px); }
	#sign_basis { width: calc(50px + sign(50% - 100px) * 10px); }
	#angle_scale { width: 10px; height: 15px; transform: rotate(atan2(1, 0)) scale(sqrt(4)); }
	</style>"""
	var index: int
	for expression: String in widths:
		html += '<div class="probe" id="math_%d" style="width: 19px; width: %s"></div>' % [index, expression]
		index += 1
	html += '<div class="probe" id="responsive"></div><div class="probe" id="sign_basis"></div><div id="angle_scale"></div>'
	_check(layout.parse(html) == OK and layout.layout(Vector2i(180, 420)) == OK, "Lay out CSS math function fixture")
	index = 0
	for expression: String in widths:
		var actual: float = _rect(layout, "math_%d" % index).size.x
		_check(is_equal_approx(actual, widths[expression]),
			"%s gives width %s, expected %s" % [expression, actual, widths[expression]])
		index += 1
	_check(is_equal_approx(_rect(layout, "responsive").size.x, 60.0), "round() uses the containing width for percentages")
	_check(is_equal_approx(_rect(layout, "sign_basis").size.x, 40.0), "sign() keeps the basis of a percentage-length argument")
	_check(_rect(layout, "angle_scale").size.is_equal_approx(Vector2(30.0, 20.0)), "Transforms use computed numbers and angles")
	_check(layout.layout(Vector2i(300, 420)) == OK, "Relayout math functions at a larger width")
	_check(is_equal_approx(_rect(layout, "responsive").size.x, 110.0), "round() follows viewport changes")
	_check(is_equal_approx(_rect(layout, "sign_basis").size.x, 60.0), "sign() reevaluates when the basis crosses zero")
	_check(_has_diagnostic(layout, "width", "pow(2px , 2)", "inline_style"), "Math type errors retain declaration diagnostics")


func _check_length_units() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var html: String = """<!doctype html>
<style>
html { font: calc(150% + 1rem) / calc(1em + 2px) monospace; }
body { margin: 0; }
.probe { height: 2px; }
#media { width: 10px; }
@media (min-width: 20rem) { #media { width: 20px; } }
#dynamic_media { width: 10px; }
@media (calc(100dvh + 1px) < width <= 110lvw) { #dynamic_media { width: 30px; } }
.line { font: 20px / calc(3 / 2) sans-serif; }
#parent_line { font-size: .5lh; line-height: calc(1lh + 2px); width: 1em; height: 1lh; }
#inherited_line { font-size: 10px; width: 1rlh; height: 1lh; }
#numeric_line { font-size: 10px; width: 1rem; height: 1lh; }
</style>
<div id="absolute" class="probe" style="width: 127Q"></div>
<div id="dynamic_media" class="probe"></div><div id="media" class="probe"></div>
<div id="math" class="probe" style="--span: 20svw; width: calc(var(--span) + 10dvb + 127q)"></div>
<div class="line">
  <div id="parent_line"><div id="inherited_line"></div></div>
  <div id="numeric_line"></div>
</div>
<div id="root_units" style="width: calc(1rch + 1rcap); height: calc(1rex + 1ric)"></div>
<div id="font_units" style="width: calc(1ch + 1cap); height: calc(1ex + 1ic)"></div>
<div id="normal_line" style="font-size: 20px; line-height: normal; height: 1lh"></div>
<div id="emphasis_line" style="font-size: 20px; line-height: normal; text-emphasis: dot; height: 1lh"></div>
"""
	var axis_units: Dictionary[String, String] = {
		"w": "w",
		"h": "h",
		"i": "w",
		"b": "h",
		"min": "min",
		"max": "max",
	}
	for prefix: String in ["v", "sv", "lv", "dv"]:
		for axis: String in axis_units:
			var unit: String = prefix + axis
			html += '<div id="%s" class="probe" style="width: 10%s"></div>' % [unit, unit]
	_check(layout.parse(html) == OK, "Parse viewport and font-relative lengths")
	for viewport: Vector2i in [Vector2i(400, 240), Vector2i(200, 360), Vector2i(420, 410)]:
		_check(layout.layout(viewport) == OK, "Lay out length units at %s" % viewport)
		var reference: Dictionary[String, float] = {
			"w": viewport.x * 0.1,
			"h": viewport.y * 0.1,
			"min": mini(viewport.x, viewport.y) * 0.1,
			"max": maxi(viewport.x, viewport.y) * 0.1,
		}
		for prefix: String in ["v", "sv", "lv", "dv"]:
			for axis: String in axis_units:
				var unit: String = prefix + axis
				var expected: float = reference[axis_units[axis]]
				_check(is_equal_approx(_rect(layout, unit).size.x, expected), "%s resolves at %s" % [unit, viewport])
		var expected_math: float = viewport.x * 0.2 + viewport.y * 0.1 + 120.0
		_check(is_equal_approx(_rect(layout, "math").size.x, expected_math), "Math mixes viewport and absolute units")
		_check(is_equal_approx(_rect(layout, "media").size.x, 20.0 if viewport.x >= 320 else 10.0),
			"Media queries resolve rem against the initial font size")
		_check(is_equal_approx(_rect(layout, "dynamic_media").size.x, 30.0 if viewport.x > viewport.y + 1 else 10.0),
			"Media query range recalculates both viewport-dependent bounds")
		_check(_rect(layout, "parent_line").size.is_equal_approx(Vector2(15.0, 32.0)), "lh uses parent metrics in font-size and line-height")
		_check(_rect(layout, "inherited_line").size.is_equal_approx(Vector2(42.0, 32.0)), "Inherited length line-height is already computed")
		_check(_rect(layout, "numeric_line").size.is_equal_approx(Vector2(40.0, 15.0)), "Inherited unitless line-height uses the child's font size")
		var root_units: Vector2 = _rect(layout, "root_units").size
		_check(root_units.x > 0.0 and root_units.y > 0.0 and root_units.is_equal_approx(_rect(layout, "font_units").size),
			"Local and root glyph units resolve to the same metrics with the same font")
		var normal: float = _rect(layout, "normal_line").size.y
		_check(normal > 0.0 and is_equal_approx(normal, _rect(layout, "emphasis_line").size.y),
			"Normal lh uses the font metrics without extra space added by text emphasis")
	_check(is_equal_approx(_rect(layout, "absolute").size.x, 120.0), "127Q equals 120 CSS pixels")
	_check(layout.get_css_diagnostics().is_empty(), "Supported lengths leave no diagnostics")


func _check_grid_intrinsic_tracks() -> void:
	var font_fixture: HTMLDocument = load("{FONT_FACE_UID}")
	_check(font_fixture != null, "Load the intrinsic sizing font fixture")
	if font_fixture == null:
		return
	var assets: Dictionary = font_fixture.get("_local_fonts")
	var font: FontFile = assets["res://fonts/cour.ttf"]
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_local_fonts", assets)
	document.text = """<!doctype html><style>
		@font-face { font-family: GridMono; src: url(fonts/cour.ttf); }
		body { margin: 0; font: 20px / 24px GridMono; }
		.grid { display: grid; width: 40ch; column-gap: 1ch; }
		.inline { display: inline-grid; }
		.basic { grid-template-columns: min-content max-content fit-content(calc(4ch + 2ch)) auto; }
		.floors { grid-template-columns: repeat(3, fit-content(2ch)); }
		.relaxed { min-width: 0; }
		.scrollable { overflow: auto; }
		.bounds { grid-template-columns: minmax(min-content, 2ch) minmax(max-content, min-content)
			minmax(0, min-content) minmax(min-content, 1fr); }
		.percent { grid-template-columns: fit-content(25%); }
		.capped_spans { grid-template-columns: fit-content(2ch) fit-content(4ch); }
		#typed_spans { grid-template-columns: min-content max-content; }
		.span { grid-column: 1 / -1; }
		.fractional { width: 12ch; grid-template-columns: min-content minmax(min-content, 1fr); }
		#flex_span { grid-column: 1 / -1; }
		.rows { height: 200px; grid-template-rows: min-content max-content fit-content(36px) auto; }
		.rows .relaxed { min-height: 0; }
		.implicit { grid: auto-flow max-content / 3ch; grid-auto-columns: min-content fit-content(6ch); }
		.auto_repeat { width: 18ch; grid-template-columns: repeat(auto-fill, minmax(5ch, max-content)); }
		.invalid { grid-template-columns: 4ch; grid-template-columns: repeat(auto-fit, max-content); }
		.bad_cap { grid-template-columns: 4ch; grid-template-columns: fit-content(-10px); }
		#condition { width: 1ch; height: 1px; }
		@supports (grid-template-columns: repeat(2, fit-content(6ch) min-content)) and
			(not (grid-template-columns: repeat(auto-fill, fit-content(6ch)))) and
			(not (grid-template-columns: minmax(fit-content(6ch), 1fr))) { #condition { width: 5ch; } }
	</style>
	<div class="grid basic">
		<div id="min">AAAA BBBB</div><div id="max">AAAA BBBB</div>
		<div id="fit">AAAA BBBB</div><div id="auto">AAAA BBBB</div>
	</div>
	<div class="grid floors">
		<div id="floor">AAAA BBBB</div><div id="relaxed" class="relaxed">AAAA BBBB</div>
		<div id="scrollable" class="scrollable">AAAA BBBB</div>
	</div>
	<div class="grid bounds">
		<div id="min_bound" class="relaxed">AAAA BBBB</div><div id="max_bound">AAAA BBBB</div>
		<div id="min_limit">AAAA BBBB</div><div id="fraction">AAAA BBBB</div>
	</div>
	<div class="grid percent"><div id="percent">AAAA BBBB CCCC</div></div>
	<div class="inline percent" id="inline_percent"><div id="inline_track">AAAA BBBB CCCC</div></div>
	<div class="grid capped_spans">
		<div id="cap_a"></div><div id="cap_b"></div><div class="span">AAAA BBBB CCCC</div>
	</div>
	<div class="grid capped_spans">
		<div id="floor_a"></div><div id="floor_b"></div><div class="span">AAAAAAAA</div>
	</div>
	<div class="grid" id="typed_spans">
		<div id="span_min"></div><div id="span_max"></div><div class="span">AAAA BBBB</div>
	</div>
	<div class="grid fractional">
		<div id="flex_min"></div><div id="flex_track"></div><div id="flex_span">AAAAAAAA</div>
	</div>
	<div class="grid rows">
		<div id="row_min">A<br>B</div><div id="row_max">A<br>B</div>
		<div id="row_fit" class="relaxed">A<br>B</div><div id="row_auto">A<br>B</div>
	</div>
	<div class="grid implicit">
		<div id="implicit_min" style="grid-column: 2">AAAA BBBB</div>
		<div id="implicit_fit" style="grid-column: 3">AAAA BBBB</div>
		<div id="implicit_repeat" style="grid-column: 4">AAAA BBBB</div>
	</div>
	<div class="grid auto_repeat">
		<div id="repeat_first">AA</div><div>AA</div><div id="repeat_third">AA</div><div id="repeat_fourth">AA</div>
	</div>
	<div class="grid invalid"><div id="invalid"></div></div>
	<div class="grid bad_cap"><div id="bad_cap"></div></div><div id="condition"></div>
	"""
	var layout: HTMLLayout = document.create_layout()
	layout.css_diagnostics_enabled = true
	_check(layout.layout(Vector2i(850, 900)) == OK, "Lay out intrinsic Grid track sizes")
	var ch: float = font.get_char_size(48, 20).x
	var widths: Dictionary[String, float] = {
		"min": 4.0,
		"max": 9.0,
		"fit": 6.0,
		"auto": 18.0,
		"floor": 4.0,
		"relaxed": 2.0,
		"scrollable": 2.0,
		"min_bound": 4.0,
		"max_bound": 9.0,
		"min_limit": 4.0,
		"fraction": 20.0,
		"percent": 10.0,
		"inline_percent": 14.0,
		"inline_track": 4.0,
		"cap_a": 2.0,
		"cap_b": 4.0,
		"floor_a": 2.5,
		"floor_b": 4.5,
		"span_min": 1.5,
		"span_max": 6.5,
		"flex_min": 0.0,
		"flex_track": 11.0,
		"implicit_min": 4.0,
		"implicit_fit": 6.0,
		"implicit_repeat": 4.0,
		"repeat_first": 5.0,
		"invalid": 4.0,
		"bad_cap": 4.0,
		"condition": 5.0,
	}
	for id: String in widths:
		var width: float = _rect(layout, id).size.x
		_check(absf(width - widths[id] * ch) < 0.15, "Grid %s width is %s, expected %s" % [id, width, widths[id] * ch])
	_check(_rect(layout, "repeat_third").position.y == _rect(layout, "repeat_first").position.y and
		_rect(layout, "repeat_fourth").position.y > _rect(layout, "repeat_first").position.y,
		"Auto-repeat accepts an intrinsic maximum with a fixed minimum")
	var heights: Dictionary[String, float] = {"row_min": 48.0, "row_max": 48.0, "row_fit": 36.0, "row_auto": 68.0,}
	for id: String in heights:
		var height: float = _rect(layout, id).size.y
		_check(absf(height - heights[id]) < 0.15, "Grid %s height is %s, expected %s" % [id, height, heights[id]])
	_check(_has_diagnostic(layout, "grid-template-columns", "repeat(auto-fit , max-content)", "style_element") and
		_has_diagnostic(layout, "grid-template-columns", "fit-content(-10px)", "style_element"),
		"Invalid intrinsic track syntax leaves the valid declaration in place: %s" % [layout.get_css_diagnostics()])


func _check_intrinsic_box_widths() -> void:
	var font_fixture: HTMLDocument = load("{FONT_FACE_UID}")
	_check(font_fixture != null, "Load the intrinsic sizing font fixture")
	if font_fixture == null:
		return
	var assets: Dictionary = font_fixture.get("_local_fonts")
	var font: FontFile = assets["res://fonts/cour.ttf"]
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_local_fonts", assets)
	document.text = """<!doctype html><style>
		@font-face { font-family: MeasureMono; src: url(fonts/cour.ttf); }
		body { margin: 0; font: 20px / 24px MeasureMono; }
		.min { width: min-content; }
		.max { width: max-content; }
		.fit { width: fit-content; }
		.narrow { width: 6ch; }
		.medium { width: 12ch; }
		table { border-spacing: 0; font: inherit; }
		td { padding: 0; }
		@supports (inline-size: stretch(70%)) { #logical { inline-size: max-content !important; } }
	</style>
	<div id="min" class="min">AAAA BBBB</div>
	<div id="max" class="max">AAAA BBBB</div>
	<div class="narrow"><div id="fit" class="fit">AAAA BBBB</div></div>
	<div style="width: 2ch"><div id="fit_floor" class="fit">AAAA BBBB</div></div>
	<div id="fit_ceiling" class="fit">AAAA BBBB</div>
	<div id="fit_limit" style="--cap: calc(4ch + 2ch); width: fit-content(var(--cap))">AAAA BBBB</div>
	<div class="medium"><div id="fit_percent" style="width: fit-content(50%)">AAAA BBBB</div></div>
	<div id="min_bound" style="width: 1ch; min-width: max-content">AAAA BBBB</div>
	<div id="max_bound" style="width: 10ch; max-width: min-content">AAAA BBBB</div>
	<div id="both_bounds" style="width: 10ch; min-width: max-content; max-width: min-content">AAAA BBBB</div>
	<div id="border_box" class="max" style="box-sizing: border-box; padding: 0 1ch; border: 2px solid">AAAA BBBB</div>
	<div class="medium"><div id="stretch" style="width: stretch; margin: 0 1ch; padding: 0 1ch">AAAA BBBB</div></div>
	<div class="medium"><div id="stretch_limit" style="width: stretch(75%); margin: 0 1ch; padding: 0 1ch">AAAA BBBB</div></div>
	<div id="fit_outlines" style="width: fit-content(9ch); margin: 0 1ch; padding: 0 1ch">AAAA BBBB</div>
	<div id="percentage_child" class="max"><div style="width: 50%">AAAA BBBB</div></div>
	<div id="percentage_minimum" class="max"><div style="min-width: calc(50% + 1ch)">AAAA BBBB</div></div>
	<div id="flex_min" class="min" style="display: flex; flex-wrap: wrap; gap: 1ch">
		<div>AAAA</div><div>BBBBB</div></div>
	<div id="grid_min" class="min" style="display: grid; grid-template-columns: auto auto; gap: 1ch">
		<div>AAAA</div><div>BBBBB</div></div>
	<div class="narrow" style="display: flex"><div id="flex_shrink" class="max">AAAA BBBB</div></div>
	<div class="medium" style="display: flex"><div id="flex_basis_max" class="min" style="flex: 0 0 max-content">AAAA BBBB</div></div>
	<div class="medium" style="display: flex">
		<div id="flex_bounds" style="width: 4ch; min-width: max-content; max-width: min-content">AAAA BBBB</div></div>
	<div class="medium" style="display: flex; flex-direction: column">
		<div id="flex_column_item" class="fit">AAAA BBBB</div></div>
	<div style="display: grid; grid-template-columns: 12ch">
		<div id="grid_item" class="fit">AAAA BBBB</div></div>
	<div style="display: grid; grid-template-columns: min-content">
		<div class="max" style="display: grid">AAAA BBBB</div><div id="grid_contribution">probe</div></div>
	<table id="table_min" class="min"><tr><td>AAAA BBBB</td></tr></table>
	<table id="table_max" class="max"><tr><td>AAAA BBBB</td></tr></table>
	<table id="table_max_bound" style="width: 14ch; max-width: max-content"><tr><td>AAAA BBBB</td></tr></table>
	<table id="table_min_bound" style="width: 4ch; min-width: max-content"><tr><td>AAAA BBBB</td></tr></table>
	<div id="logical" style="width: 2ch; max-inline-size: 6ch; --minimum: 5ch; min-inline-size: var(--minimum);
		min-block-size: 55px; height: 20px">AAAA BBBB</div>
	<div id="logical_order" style="inline-size: max-content; width: 7ch; block-size: 66px; max-block-size: 41px">AAAA BBBB</div>
	<div style="width: 50%"><div id="responsive" class="fit">AAAA BBBB</div></div>
	<div id="viewport_measure" class="max"><div style="width: 40vw; height: 1px"></div></div>
	<div class="medium" style="position: relative; height: 100px">
		<div id="positioned_max" class="max" style="position: absolute; left: 3ch; right: 3ch; top: 0">AAAA BBBB</div>
		<div id="positioned_fit" class="fit" style="position: absolute; left: 3ch; right: 3ch; bottom: 0">AAAA BBBB</div>
		<div id="positioned_cap" style="position: absolute; left: 0; right: 0; top: 24px; max-width: min-content">AAAA BBBB</div>
		<div id="positioned_percent" style="position: absolute; left: 3ch; right: 3ch; top: 0; width: stretch(75%)">AAAA BBBB</div>
	</div>
	<img id="image_min" src="intrinsic-image.png" style="width: min-content">
	<div class="narrow" style="display: flex">
		<img id="image_flex" src="intrinsic-image.png" style="width: max-content; min-width: 0"></div>
	<div id="invalid" style="width: 5ch; width: fit-content(-1ch)">AAAA BBBB</div>"""
	document.text += '<div id="deep" class="max">' + '<div class="max">'.repeat(24) + "AAAA BBBB" + "</div>".repeat(25)
	var image: Image = Image.create_empty(120, 60, false, Image.FORMAT_RGBA8)
	var texture: ImageTexture = ImageTexture.create_from_image(image)
	document.set("_textures", {"res://intrinsic-image.png": texture,})
	var layout: HTMLLayout = document.create_layout()
	layout.css_diagnostics_enabled = true
	_check(layout.layout(Vector2i(600, 800)) == OK, "Lay out intrinsic box widths")
	var ch: float = font.get_char_size(48, 20).x
	var widths: Dictionary[String, float] = {
		"min": 4.0 * ch,
		"max": 9.0 * ch,
		"fit": 6.0 * ch,
		"fit_floor": 4.0 * ch,
		"fit_ceiling": 9.0 * ch,
		"fit_limit": 6.0 * ch,
		"fit_percent": 6.0 * ch,
		"min_bound": 9.0 * ch,
		"max_bound": 4.0 * ch,
		"both_bounds": 9.0 * ch,
		"border_box": 11.0 * ch + 4.0,
		"stretch": 10.0 * ch,
		"stretch_limit": 7.0 * ch,
		"fit_outlines": 7.0 * ch,
		"percentage_child": 9.0 * ch,
		"percentage_minimum": 9.0 * ch,
		"flex_min": 5.0 * ch,
		"grid_min": 10.0 * ch,
		"flex_shrink": 6.0 * ch,
		"flex_basis_max": 9.0 * ch,
		"flex_bounds": 9.0 * ch,
		"flex_column_item": 9.0 * ch,
		"grid_item": 9.0 * ch,
		"grid_contribution": 9.0 * ch,
		"table_min": 4.0 * ch,
		"table_max": 9.0 * ch,
		"table_max_bound": 9.0 * ch,
		"table_min_bound": 9.0 * ch,
		"logical": 6.0 * ch,
		"logical_order": 7.0 * ch,
		"responsive": 9.0 * ch,
		"viewport_measure": 240.0,
		"positioned_max": 9.0 * ch,
		"positioned_fit": 6.0 * ch,
		"positioned_cap": 4.0 * ch,
		"positioned_percent": 9.0 * ch,
		"image_min": 120.0,
		"image_flex": 6.0 * ch,
		"invalid": 5.0 * ch,
		"deep": 9.0 * ch,
	}
	for id: String in widths:
		var actual: float = _rect(layout, id).size.x
		_check(absf(actual - widths[id]) <= 0.1, "Intrinsic width for %s is %s, expected %s" % [id, actual, widths[id]])
	_check(is_equal_approx(_rect(layout, "min").size.y, 48.0), "Min-content wraps at the space")
	_check(is_equal_approx(_rect(layout, "max").size.y, 24.0), "Max-content keeps both words on one line")
	_check(is_equal_approx(_rect(layout, "logical").size.y, 55.0), "Logical minimum block size overrides the smaller height")
	_check(is_equal_approx(_rect(layout, "logical_order").size.y, 41.0), "Logical maximum block size caps block-size")
	_check(is_equal_approx(_rect(layout, "positioned_fit").size.y, 48.0), "Positioned fit-content wraps within the insets")
	_check(is_equal_approx(_rect(layout, "image_flex").size.y, 3.0 * ch), "Flex shrinking preserves the image's natural ratio")
	_check(_has_diagnostic(layout, "width", "fit-content(-1ch)", "inline_style"), "Report a negative fit-content limit")
	_check(layout.layout(Vector2i(144, 800)) == OK, "Reflow intrinsic boxes after a viewport resize")
	_check(is_equal_approx(_rect(layout, "responsive").size.x, 72.0), "Fit-content tracks the resized containing block")
	_check(is_equal_approx(_rect(layout, "viewport_measure").size.x, 57.6), "Relayout remeasures changed viewport units")


func _check_aspect_ratio() -> void:
	var document: HTMLDocument = HTMLDocument.new()
	var image: Image = Image.create_empty(120, 60, false, Image.FORMAT_RGBA8)
	var texture: ImageTexture = ImageTexture.create_from_image(image)
	document.set("_textures", {"res://ratio-source.png": texture,})
	document.text = """<!doctype html><style>
		body { margin: 0; }
		.stage { width: 160px; }
		.grid { display: grid; grid-template-columns: 120px; }
		.flex { display: flex; align-items: start; }
		img { display: block; }
		@supports (aspect-ratio: auto calc(4 + 2) / 3) {
			#math { aspect-ratio: var(--ratio); }
		}
	</style>
	<div id="from_width" style="width: 120px; aspect-ratio: 2 / 1"></div>
	<div id="from_height" style="height: 45px; aspect-ratio: 2"></div>
	<div class="stage"><div id="fill" style="aspect-ratio: 1.6"></div></div>
	<div id="both" style="width: 80px; height: 55px; aspect-ratio: 2"></div>
	<div id="border" style="width: 120px; box-sizing: border-box; padding: 6px; border: 4px solid; aspect-ratio: 2"></div>
	<div id="content" style="width: 120px; padding: 6px; border: 4px solid; aspect-ratio: 2"></div>
	<div id="auto_border" style="width: 120px; box-sizing: border-box; padding: 10px; aspect-ratio: auto 2"></div>
	<div id="math" style="width: 120px; --ratio: calc(4 + 2) / 3 auto"></div>
	<div id="zero" style="width: 120px; aspect-ratio: 10 / 0"><div style="height: 10px"></div></div>
	<div id="invalid" style="width: 120px; aspect-ratio: 2; aspect-ratio: -3 / 1"></div>
	<div id="wide" style="height: 25px; aspect-ratio: 2"><div style="width: 130px; height: 10px"></div></div>
	<div id="wide_zero" style="height: 25px; aspect-ratio: 2; min-width: 0"><div style="width: 130px; height: 10px"></div></div>
	<div id="tall" style="width: 100px; aspect-ratio: 2"><div style="height: 90px"></div></div>
	<div id="hidden" style="width: 100px; aspect-ratio: 2; overflow: hidden"><div style="height: 90px"></div></div>
	<div id="tall_zero" style="width: 100px; aspect-ratio: 2; min-height: 0"><div style="height: 90px"></div></div>
	<div class="stage"><div id="max_height" style="aspect-ratio: 2; max-height: 35px"></div></div>
	<div id="min_width" style="height: 25px; aspect-ratio: 2; min-width: 80px"></div>
	<div style="height: 100px"><div id="percent_height" style="height: 50%; aspect-ratio: 3"></div></div>
	<div style="width: 100px; height: 100px; aspect-ratio: 2">
		<div id="inherit" style="height: 20px; aspect-ratio: inherit"></div></div>
	<span id="inline_block" style="display: inline-block; width: 80px; aspect-ratio: 2"></span>
	<div class="grid" style="grid-template-rows: 180px 60px">
		<div id="grid_normal" style="aspect-ratio: 2"></div>
		<div id="grid_fixed_height" style="height: 40px; aspect-ratio: 2"></div>
	</div>
	<div class="grid" id="grid_container" style="width: 150px; aspect-ratio: 3; align-content: start"><div></div></div>
	<div class="flex" style="width: 60px"><div id="flex_shrink" style="width: 100px; aspect-ratio: 2"></div></div>
	<div class="flex"><div id="flex_height" style="height: 40px; aspect-ratio: 2"></div></div>
	<div style="display: flex; flex-direction: column; width: 120px"><div id="flex_column" style="aspect-ratio: 2"></div></div>
	<div class="stage" style="position: relative; height: 120px">
		<div id="absolute_height" style="position: absolute; bottom: 0; width: 80px; aspect-ratio: 2"></div>
		<div id="absolute_width" style="position: absolute; right: 0; height: 40px; aspect-ratio: 2"></div>
		<div id="absolute_four_insets" style="position: absolute; inset: 10px; aspect-ratio: 2"></div>
		<div id="absolute_vertical_insets" style="position: absolute; inset: 10px auto 10px 0; aspect-ratio: 2"></div>
	</div>
	<div id="fixed_insets" style="position: fixed; inset: 12px 460px auto 20px; aspect-ratio: 2"></div>
	<table id="table_ratio" style="width: 120px; aspect-ratio: 2; border-spacing: 0"><tr><td>X</td></tr></table>
	<div id="inline_grid_ratio" style="display: inline-grid; aspect-ratio: 2"><div style="width: 80px; height: 10px"></div></div>
	<div id="percentage_child" style="width: 120px; aspect-ratio: 2"><div id="ratio_child" style="height: 50%"></div></div>
	<img id="image_override" src="ratio-source.png" style="width: 80px; aspect-ratio: 1">
	<img id="image_natural" src="ratio-source.png" style="width: 80px; aspect-ratio: 1 auto">
	<img id="image_auto" src="ratio-source.png" style="aspect-ratio: 1">
	<img id="image_border" src="ratio-source.png" style="height: 80px; box-sizing: border-box; padding: 10px; aspect-ratio: 2">
	<img id="image_missing" src="missing.png" style="width: 80px; aspect-ratio: auto 2">
	<img id="image_inline" src="ratio-source.png" style="display: inline; width: 80px; aspect-ratio: 1">
	<img id="image_hint" src="missing.png" width="80" height="20" style="width: 120px; height: auto; aspect-ratio: revert">
	<img id="image_hint_natural" src="ratio-source.png" width="80" height="20" style="width: 120px; height: auto">
	<img id="image_hint_override" src="missing.png" width="80" height="20" style="width: 120px; height: auto; aspect-ratio: 1">
	<div class="flex" style="width: 200px"><img id="flex_image_auto" src="ratio-source.png" style="aspect-ratio: 1"></div>"""
	var layout: HTMLLayout = document.create_layout()
	layout.css_diagnostics_enabled = true
	_check(layout.layout(Vector2i(600, 800)) == OK, "Lay out preferred aspect ratios")
	var sizes: Dictionary[String, Vector2] = {
		"from_width": Vector2(120.0, 60.0),
		"from_height": Vector2(90.0, 45.0),
		"fill": Vector2(160.0, 100.0),
		"both": Vector2(80.0, 55.0),
		"border": Vector2(120.0, 60.0),
		"content": Vector2(140.0, 80.0),
		"auto_border": Vector2(120.0, 70.0),
		"math": Vector2(120.0, 60.0),
		"zero": Vector2(120.0, 10.0),
		"invalid": Vector2(120.0, 60.0),
		"wide": Vector2(130.0, 25.0),
		"wide_zero": Vector2(50.0, 25.0),
		"tall": Vector2(100.0, 90.0),
		"hidden": Vector2(100.0, 50.0),
		"tall_zero": Vector2(100.0, 50.0),
		"max_height": Vector2(70.0, 35.0),
		"min_width": Vector2(80.0, 25.0),
		"percent_height": Vector2(150.0, 50.0),
		"inherit": Vector2(40.0, 20.0),
		"inline_block": Vector2(80.0, 40.0),
		"grid_normal": Vector2(120.0, 60.0),
		"grid_fixed_height": Vector2(80.0, 40.0),
		"grid_container": Vector2(150.0, 50.0),
		"flex_shrink": Vector2(60.0, 30.0),
		"flex_height": Vector2(80.0, 40.0),
		"flex_column": Vector2(120.0, 60.0),
		"absolute_height": Vector2(80.0, 40.0),
		"absolute_width": Vector2(80.0, 40.0),
		"absolute_four_insets": Vector2(140.0, 70.0),
		"absolute_vertical_insets": Vector2(200.0, 100.0),
		"fixed_insets": Vector2(120.0, 60.0),
		"table_ratio": Vector2(120.0, 60.0),
		"inline_grid_ratio": Vector2(80.0, 40.0),
		"ratio_child": Vector2(120.0, 30.0),
		"image_override": Vector2(80.0, 80.0),
		"image_natural": Vector2(80.0, 40.0),
		"image_auto": Vector2(120.0, 120.0),
		"image_border": Vector2(160.0, 80.0),
		"image_missing": Vector2(80.0, 40.0),
		"image_inline": Vector2(80.0, 80.0),
		"image_hint": Vector2(120.0, 30.0),
		"image_hint_natural": Vector2(120.0, 60.0),
		"image_hint_override": Vector2(120.0, 120.0),
		"flex_image_auto": Vector2(120.0, 120.0),
	}
	for id: String in sizes:
		var actual: Vector2 = _rect(layout, id).size
		_check(actual.distance_to(sizes[id]) <= 0.1, "Aspect ratio for %s produced %s, expected %s" % [id, actual, sizes[id]])
	_check(_has_diagnostic(layout, "aspect-ratio", "-3 / 1", "inline_style"), "Diagnose a negative literal ratio")
	var fill: HTMLElement = layout.get_element_by_id("fill")
	_check(fill.set_attribute("style", "aspect-ratio: 2") == OK and layout.layout(Vector2i(600, 800)) == OK,
		"Reflow after editing the preferred aspect ratio")
	_check(fill.get_bounding_rect().size.is_equal_approx(Vector2(160.0, 80.0)), "A ratio edit updates the existing element")


func _check_object_sizing() -> void:
	var red: Color = Color("#ef3340")
	var blue: Color = Color("#3258df")
	var gold: Color = Color("#ffbc36")
	var green: Color = Color("#1eb677")
	var background: Color = Color("#493457")
	var source: Image = Image.create_empty(40, 20, false, Image.FORMAT_RGBA8)
	source.fill_rect(Rect2i(0, 0, 20, 10), red)
	source.fill_rect(Rect2i(20, 0, 20, 10), blue)
	source.fill_rect(Rect2i(0, 10, 20, 10), gold)
	source.fill_rect(Rect2i(20, 10, 20, 10), green)
	var texture: ImageTexture = ImageTexture.create_from_image(source)
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_textures", { "res://object.png": texture, })
	document.text = """<!doctype html><style>
		body { margin: 0; display: grid; grid-template-columns: repeat(6, 82px); grid-auto-rows: 72px;
			align-items: start; justify-items: start; }
		img { display: block; width: 40px; height: 40px; background: #493457; }
		.large { width: 60px; height: 40px; }
		#upscale { object-fit: cover; }
		@supports (object-fit: scale-down cover) and (object-position: bottom 2px right 10%) {
			#upscale { object-fit: scale-down cover; }
		}
	</style>
	<img id="fill" src="object.png" style="width: 20px">
	<img id="contain" src="object.png" style="object-fit: contain">
	<img id="cover" src="object.png" style="object-fit: cover">
	<img id="cover_left" src="object.png" style="object-fit: cover; object-position: left">
	<img id="cover_right" src="object.png" style="object-fit: cover; object-position: top right">
	<img id="down_large" class="large" src="object.png" style="object-fit: scale-down">
	<img id="down_small" src="object.png" style="width: 20px; height: 20px; object-fit: contain scale-down">
	<img id="upscale" class="large" src="object.png">
	<img id="none" class="large" src="object.png" style="object-fit: none; object-position: 1em 25%; font-size: 12px">
	<img id="edge" class="large" src="object.png" style="height: 46px; object-fit: none;
		--position: bottom calc(4px + 2px) right 25%; object-position: var(--position)">
	<img id="negative_space" src="object.png" style="object-fit: cover; object-position: calc(100% - 4px) center">
	<img id="fill_offset" src="object.png" style="object-position: 6px 0">
	<img id="rounded" src="object.png" style="object-fit: cover; border-radius: 15px">
	<img id="flipped" src="object.png" style="object-fit: cover; transform: scaleX(-1)">
	<img id="padded" src="object.png" style="padding: 5px; border: 3px solid black; object-fit: contain">
	<img id="invalid" src="object.png" style="object-fit: contain; object-fit: fill scale-down;
		object-position: bottom; object-position: right 3px bottom">
	<img id="box_ratio" src="object.png" style="height: auto; aspect-ratio: 1; object-fit: contain">
	<img id="none_crop" src="object.png" style="width: 20px; height: 10px; object-fit: none; object-position: left bottom">
	<div id="window" style="width: 28px; height: 28px; overflow: hidden; border-radius: 9px">
		<img src="object.png" style="object-fit: cover">
	</div>"""
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(520, 345)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(520.0, 260.0)
	label.document = document
	viewport.add_child(label)
	var markdown: MarkdownLabel = MarkdownLabel.new()
	markdown.position = Vector2(0.0, 264.0)
	markdown.size = Vector2(220.0, 75.0)
	for icon: StringName in [&"task_checked", &"task_unchecked", &"file_broken"]:
		markdown.add_theme_icon_override(icon, texture)
	markdown.parse_html = true
	markdown.text = """
<div><img src="object-fit.svg" style="display: block; width: 40px; height: 40px;
  object-fit: contain; object-position: bottom; background: #493457"></div>
"""
	viewport.add_child(markdown)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var painted: Image = viewport.get_texture().get_image()
	var samples: Array[Array] = [
		["fill", Vector2(4.0, 5.0), red],
		["fill", Vector2(15.0, 32.0), green],
		["contain", Vector2(4.0, 5.0), background],
		["contain", Vector2(4.0, 14.0), red],
		["cover", Vector2(5.0, 5.0), red],
		["cover", Vector2(32.0, 5.0), blue],
		["cover_left", Vector2(30.0, 5.0), red],
		["cover_right", Vector2(5.0, 5.0), blue],
		["down_large", Vector2(4.0, 13.0), background],
		["down_large", Vector2(12.0, 13.0), red],
		["down_small", Vector2(4.0, 2.0), background],
		["down_small", Vector2(4.0, 7.0), red],
		["upscale", Vector2(5.0, 14.0), background],
		["upscale", Vector2(14.0, 14.0), red],
		["none", Vector2(10.0, 8.0), background],
		["none", Vector2(14.0, 8.0), red],
		["edge", Vector2(13.0, 24.0), background],
		["edge", Vector2(18.0, 24.0), red],
		["edge", Vector2(18.0, 42.0), background],
		["negative_space", Vector2(2.0, 5.0), blue],
		["negative_space", Vector2(38.0, 5.0), background],
		["fill_offset", Vector2(3.0, 5.0), background],
		["fill_offset", Vector2(9.0, 5.0), red],
		["rounded", Vector2(10.0, 10.0), red],
		["flipped", Vector2(8.0, 5.0), blue],
		["padded", Vector2(5.0, 20.0), background],
		["padded", Vector2(12.0, 20.0), red],
		["invalid", Vector2(4.0, 16.0), background],
		["invalid", Vector2(4.0, 23.0), red],
		["box_ratio", Vector2(4.0, 5.0), background],
		["box_ratio", Vector2(4.0, 14.0), red],
		["none_crop", Vector2(6.0, 4.0), gold],
	]
	for sample: Array in samples:
		var id: String = sample[0]
		var offset: Vector2 = sample[1]
		var expected: Color = sample[2]
		_check_pixel(painted, _label_rect(label, id).position + offset, expected, "Object paint for %s at %s" % [id, offset])
	_check(_label_rect(label, "box_ratio").size.is_equal_approx(Vector2(40.0, 40.0)),
		"Fitting an object preserves the ratio-sized element box")
	var rounded_corner: Vector2i = Vector2i(_label_rect(label, "rounded").position + Vector2.ONE)
	_check(not painted.get_pixelv(rounded_corner).is_equal_approx(red), "Rounded clipping masks an overflowing object corner")
	var window: Vector2 = _label_rect(label, "window").position
	_check_pixel(painted, window + Vector2(8.0, 8.0), red, "The fitted image draws inside an ancestor's rounded clip")
	_check(not _same_color(painted.get_pixelv(Vector2i(window + Vector2(32.0, 10.0))), blue),
		"The ancestor clips the image outside its box")
	var markdown_image: Rect2i = _color_bounds(painted, Rect2i(0, 264, 220, 75), Color("#c5692b"))
	_check(markdown_image.size == Vector2i(40, 20), "Markdown HTML fits the imported SVG at its natural proportions")
	var layout: HTMLLayout = document.create_layout()
	layout.css_diagnostics_enabled = true
	_check(layout.layout(Vector2i(520, 260)) == OK, "Lay out the object diagnostics fixture")
	_check(_has_diagnostic(layout, "object-fit", "fill scale-down", "inline_style"), "Report an invalid fit combination")
	_check(_has_diagnostic(layout, "object-position", "right 3px bottom", "inline_style"), "Reject a three-component position")
	var cover: HTMLElement = label.get_element_by_id("cover_left")
	var before: Rect2 = _label_rect(label, "cover_left")
	_check(cover.set_attribute("style", "object-fit: cover; object-position: right") == OK, "Update an image's object position")
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	_check(_label_rect(label, "cover_left").is_equal_approx(before), "Changing object position preserves the element bounds")
	_check_pixel(viewport.get_texture().get_image(), before.position + Vector2(30.0, 5.0), blue,
		"An object-position edit changes the crop immediately")
	viewport.queue_free()


func _check_image_rendering() -> void:
	var source: Image = Image.create_empty(3, 2, false, Image.FORMAT_RGBA8)
	var colors: Array[Color] = [Color.RED, Color.LIME, Color.BLUE, Color.YELLOW, Color.CYAN, Color.MAGENTA,]
	for index: int in colors.size():
		source.set_pixel(index % 3, floori(float(index) / 3.0), colors[index])
	var texture: ImageTexture = ImageTexture.create_from_image(source)
	var sheet: Image = Image.create_empty(5, 4, false, Image.FORMAT_RGBA8)
	sheet.fill(Color.WHITE)
	sheet.blit_rect(source, Rect2i(0, 0, 3, 2), Vector2i.ONE)
	var atlas: AtlasTexture = AtlasTexture.new()
	atlas.atlas = ImageTexture.create_from_image(sheet)
	atlas.region = Rect2(1.0, 1.0, 3.0, 2.0)
	var mipmap_image: Image = Image.create_empty(8, 8, false, Image.FORMAT_RGBA8)
	mipmap_image.fill(Color.BLUE)
	mipmap_image.fill_rect(Rect2i(3, 3, 2, 2), Color.RED)
	_check(mipmap_image.generate_mipmaps() == OK, "Generate the mipmap fixture")
	var mipmap_texture: ImageTexture = ImageTexture.create_from_image(mipmap_image)
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_textures", {
		"res://sampling.png": texture,
		"res://atlas.png": atlas,
		"res://mipmap.png": mipmap_texture,
	})
	document.text = """<!doctype html><style>
		body { margin: 0; background: #222222; display: grid; grid-template-columns: repeat(5, 32px);
			grid-auto-rows: 20px; align-items: start; justify-items: start; }
		img { display: block; width: 8px; height: 5px; }
		#repeated { width: 24px; height: 5px; background: url(sampling.png) repeat-x left top / 8px 5px;
			image-rendering: pixelated; }
		#inherited { --filter: pixelated; image-rendering: var(--filter); }
		#integer, #rounded { image-rendering: pixelated; width: 9px; height: 6px; }
		#rounded { width: 12px; height: 8px; border-radius: 4px; }
		.minified { width: 1px; height: 1px; image-rendering: smooth; }
		@supports (image-rendering: high-quality) { #mipmapped { image-rendering: high-quality; } }
	</style>
	<img id="crisp" src="sampling.png" style="image-rendering: crisp-edges">
	<img id="smooth" src="sampling.png" style="image-rendering: smooth">
	<div id="inherited"><img id="fractional" src="sampling.png"></div>
	<img id="integer" src="sampling.png">
	<img id="atlas" src="atlas.png" style="image-rendering: pixelated">
	<div id="repeated"></div>
	<img id="speed" src="sampling.png" style="image-rendering: optimizeSpeed">
	<img id="quality" src="sampling.png" style="image-rendering: optimizeQuality">
	<img id="zoom" src="sampling.png" style="width: 6px; height: 6px; image-rendering: pixelated;
		transform: scaleX(1.5); transform-origin: left top">
	<img id="rounded" src="sampling.png">
	<img id="linear_minified" class="minified" src="mipmap.png">
	<img id="mipmapped" class="minified" src="mipmap.png">
	<img id="invalid_sampling" src="sampling.png" style="image-rendering: crisp-edges; image-rendering: interpolate">
	<div id="generated" style="image-rendering: pixelated; transform: scale(3); transform-origin: left top">
		<style>#generated::before { content: url(sampling.png); display: block; line-height: 0; }</style>
	</div>
	<img id="automatic" src="sampling.png" style="image-rendering: auto">"""
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(170, 165)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	viewport.canvas_item_default_texture_filter = Viewport.DEFAULT_CANVAS_ITEM_TEXTURE_FILTER_NEAREST
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(165.0, 75.0)
	label.document = document
	viewport.add_child(label)
	var markdown: MarkdownLabel = MarkdownLabel.new()
	markdown.position = Vector2(0.0, 80.0)
	markdown.size = Vector2(150.0, 85.0)
	for icon: StringName in [&"task_checked", &"task_unchecked", &"file_broken"]:
		markdown.add_theme_icon_override(icon, texture)
	markdown.parse_html = true
	markdown.text = """
<div><img src="sampling.svg" style="display: block; image-rendering: pixelated; width: 9px; height: 6px"></div>
"""
	viewport.add_child(markdown)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw

	var result: Image = viewport.get_texture().get_image()
	var blended: Color = Color.RED.lerp(Color.LIME, 0.3125)
	var smooth: Color = Color.RED.lerp(Color.LIME, 0.0625).lerp(Color.YELLOW.lerp(Color.CYAN, 0.0625), 0.1)
	var samples: Array[Array] = [
		["crisp", Vector2(2.0, 1.0), Color.RED],
		["smooth", Vector2.ONE, smooth],
		["fractional", Vector2.ONE, Color.RED],
		["fractional", Vector2(2.0, 1.0), blended],
		["fractional", Vector2(0.0, 2.0), Color.RED.lerp(Color.YELLOW, 0.5)],
		["integer", Vector2(2.0, 1.0), Color.RED],
		["integer", Vector2(3.0, 1.0), Color.LIME],
		["atlas", Vector2.ZERO, Color.RED],
		["atlas", Vector2(7.0, 4.0), Color.MAGENTA],
		["repeated", Vector2(2.0, 1.0), blended],
		["repeated", Vector2(10.0, 1.0), blended],
		["repeated", Vector2(18.0, 1.0), blended],
		["speed", Vector2(2.0, 1.0), Color.RED],
		["quality", Vector2.ONE, smooth],
		["zoom", Vector2(2.0, 1.0), Color.RED],
		["zoom", Vector2(3.0, 1.0), Color.LIME],
		["rounded", Vector2(2.0, 2.0), Color.RED],
		["linear_minified", Vector2.ZERO, Color.RED],
		["invalid_sampling", Vector2(2.0, 1.0), Color.RED],
		["automatic", Vector2(2.0, 1.0), Color.RED],
	]
	for sample: Array in samples:
		var id: String = sample[0]
		var offset: Vector2 = sample[1]
		var color: Color = sample[2]
		_check_pixel(result, _label_rect(label, id).position + offset, color, "Image sampling for %s" % id)
	var rounded_corner: Color = result.get_pixelv(Vector2i(_label_rect(label, "rounded").position))
	_check(_same_color(rounded_corner, Color("#222222")), "Pixelated sampling preserves rounded clipping, got %s" % rounded_corner)
	var minified: Color = result.get_pixelv(Vector2i(_label_rect(label, "mipmapped").position))
	_check(minified.r < 0.25 and minified.b > 0.8, "High-quality minification uses available mipmaps, got %s" % minified)
	var generated: Rect2i = Rect2i(Vector2i(_label_rect(label, "generated").position), Vector2i(22, 18))
	_check(_color_bounds(result, generated, Color.RED).size == Vector2i(3, 3),
		"A generated image inherits pixelated filtering")
	var markdown_red: Rect2i = _color_bounds(result, Rect2i(0, 80, 150, 85), Color.RED)
	_check(markdown_red.size == Vector2i(3, 3), "Markdown HTML uses pixelated image filtering, got %s" % markdown_red)

	var layout: HTMLLayout = document.create_layout()
	layout.css_diagnostics_enabled = true
	_check(layout.layout(Vector2i(165, 75)) == OK, "Lay out the image-rendering diagnostics fixture")
	_check(_has_diagnostic(layout, "image-rendering", "interpolate", "inline_style"), "Report an unsupported sampling value")
	var inherited: HTMLElement = label.get_element_by_id("inherited")
	_check(inherited.set_attribute("style", "image-rendering: smooth") == OK, "Change the inherited image sampling mode")
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	_check_pixel(viewport.get_texture().get_image(), _label_rect(label, "fractional").position + Vector2.ONE, smooth,
		"Changing an ancestor's sampling mode refreshes the child image")
	viewport.queue_free()


func _check_image_set() -> void:
	var red_image: Image = Image.create_empty(20, 12, false, Image.FORMAT_RGBA8)
	red_image.fill(Color.RED)
	var green_image: Image = Image.create_empty(28, 16, false, Image.FORMAT_RGBA8)
	green_image.fill(Color.LIME)
	var blue_image: Image = Image.create_empty(24, 16, false, Image.FORMAT_RGBA8)
	blue_image.fill(Color.BLUE)
	var red_texture: ImageTexture = ImageTexture.create_from_image(red_image)
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_textures", {
		"res://red.png": red_texture,
		"res://green.png": ImageTexture.create_from_image(green_image),
		"res://blue.png": ImageTexture.create_from_image(blue_image),
		"res://vector.svg": red_texture,
	})
	document.text = """<!doctype html><style>
		body { margin: 0; display: grid; grid-template-columns: repeat(5, 34px); grid-auto-rows: 23px; }
		.probe { width: 29px; height: 20px; background-color: #444444; background-repeat: no-repeat; }
		#nearest { background-image: image-set("red.png" 2x, url(green.png) 96dpi, "blue.png" .75dppx); }
		#first { background-image: image-set("red.png" type("image/avif"), "green.png" type("IMAGE/PNG") 1x,
			"blue.png" 96dpi); }
		#math { background-image: image-set("red.png" calc(96dpi + 1dppx)); }
		#below { background-image: image-set("red.png" .25x, "blue.png" .75x); }
		#vector { background-image: image-set(url(vector.svg) 2x); }
		#shorthand { background: image-set(linear-gradient(lime, lime) 1x, "red.png" 2x) no-repeat top left / 9px 7px,
			image-set(url(blue.png) 2x) no-repeat top left / 19px 13px #444444; }
		#variable { --density: calc(96dpi + 1dppx); background-image: image-set(var(--missing, "red.png") var(--density)); }
		#invalid { background-image: url(green.png); background-image: image-set("red.png" -1x, "blue.png" 2x); }
		#invalid_var { --wrong: -1x; background-image: url(green.png); background-image: image-set("red.png" var(--wrong)); }
		#unsupported { background-image: url(green.png); background-image: image-set("red.png" type("image/avif")); }
		#zero { background-image: url(green.png); background-image: image-set("red.png" 0dppx); }
		#clamped { background-image: url(green.png); background-image: image-set("red.png" calc(0x - 1x)); }
		#missing { background-image: image-set("missing.png", "green.png" 2x); }
		#dpcm { background-image: image-set("red.png" calc(19200dpcm / 254)); }
		@supports (background-image: image-set(url(red.png) 2x)) {
			@media (resolution: calc(48dpi + .5dppx)) {
				#query { background-image: image-set("green.png"); }
			}
			@media (resolution > calc(2dppx)) { #query { background-image: url(red.png); } }
		}
	</style>"""
	for id: String in ["nearest", "first", "math", "below", "vector", "shorthand", "variable", "invalid", "invalid_var",
		"unsupported", "zero", "clamped", "missing", "dpcm", "query",]:
		document.text += '<div class="probe" id="%s"></div>' % id

	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(340, 90)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(175.0, 90.0)
	label.document = document
	viewport.add_child(label)
	var imported: HTMLDocument = ResourceLoader.load("{IMAGE_SET_UID}") as HTMLDocument
	_check(imported != null, "Load the imported image-set document by UID")
	var imported_label: HTMLLabel = HTMLLabel.new()
	if imported != null:
		var images: Dictionary = imported.get("_textures")
		_check(images.has("res://styles/set-low.svg") and images.has("res://styles/nested/set-high.svg"),
			"The importer embeds both the selected string candidate and the unused var() fallback candidate")
		imported_label.position = Vector2(180.0, 0.0)
		imported_label.size = Vector2(150.0, 80.0)
		imported_label.document = imported
		viewport.add_child(imported_label)
	else:
		imported_label.free()
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw

	var output: Image = viewport.get_texture().get_image()
	var empty: Color = Color("#444444")
	var samples: Array[Array] = [
		["nearest", Vector2(2.0, 2.0), Color.LIME],
		["first", Vector2(2.0, 2.0), Color.LIME],
		["math", Vector2(9.0, 5.0), Color.RED],
		["math", Vector2(10.0, 6.0), empty],
		["below", Vector2(26.0, 17.0), Color.BLUE],
		["vector", Vector2(19.0, 11.0), Color.RED],
		["shorthand", Vector2(8.0, 6.0), Color.LIME],
		["shorthand", Vector2(12.0, 9.0), Color.BLUE],
		["shorthand", Vector2(22.0, 16.0), empty],
		["variable", Vector2(9.0, 5.0), Color.RED],
		["variable", Vector2(12.0, 8.0), empty],
		["invalid", Vector2(2.0, 2.0), Color.LIME],
		["invalid_var", Vector2(2.0, 2.0), empty],
		["unsupported", Vector2(2.0, 2.0), empty],
		["zero", Vector2(2.0, 2.0), empty],
		["clamped", Vector2(2.0, 2.0), empty],
		["missing", Vector2(2.0, 2.0), empty],
		["dpcm", Vector2(9.0, 5.0), Color.RED],
		["dpcm", Vector2(12.0, 8.0), empty],
		["query", Vector2(2.0, 2.0), Color.LIME],
	]
	for sample: Array in samples:
		var id: String = sample[0]
		var offset: Vector2 = sample[1]
		var color: Color = sample[2]
		_check_pixel(output, _label_rect(label, id).position + offset, color, "Image-set choice for %s" % id)
	if imported != null:
		var point: Vector2 = imported_label.position + _label_rect(imported_label, "imported_set").position + Vector2(3.0, 3.0)
		_check_pixel(output, point, Color("#67bb39"), "An imported image-set resolves its relative URL after the linked CSS is removed")
	var layout: HTMLLayout = document.create_layout()
	layout.css_diagnostics_enabled = true
	_check(layout.layout(Vector2i(175, 90)) == OK, "Lay out the image-set diagnostics fixture")
	_check(layout.get_element_by_id("invalid").get_css_diagnostics().size() == 1 and
		layout.get_element_by_id("invalid_var").get_css_diagnostics().size() == 1,
		"Invalid literal and substituted resolutions each report one diagnostic")
	_check(layout.get_element_by_id("unsupported").get_css_diagnostics().is_empty(),
		"An unsupported MIME type leaves a syntactically valid image-set")
	viewport.queue_free()


func _check_annotated_image() -> void:
	var sprite: Image = Image.create_empty(12, 8, false, Image.FORMAT_RGBA8)
	sprite.fill_rect(Rect2i(0, 0, 4, 8), Color.RED)
	sprite.fill_rect(Rect2i(4, 0, 4, 8), Color.LIME)
	sprite.fill_rect(Rect2i(8, 0, 4, 8), Color.BLUE)
	var transparent: Image = Image.create_empty(12, 8, false, Image.FORMAT_RGBA8)
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_textures", {
		"res://sprite.png": ImageTexture.create_from_image(sprite),
		"res://alpha.png": ImageTexture.create_from_image(transparent),
	})
	document.text = """<!doctype html><style>
		body { margin: 0; display: grid; grid-template-columns: repeat(5, 36px); grid-auto-rows: 22px; }
		.probe { width: 30px; height: 19px; background-color: #404040; background-repeat: no-repeat; }
		#solid { color: lime; background: image(currentColor) no-repeat 3px 2px / 10px 6px #404040; }
		#repeat_color { background: image(blue) repeat-x 0 2px / 4px 4px #404040; }
		#missing { background-image: image("missing.png", lime); }
		#bad_fragment { background-image: image(url("sprite.png#t=1,2"), yellow); }
		#clip { background-image: image("sprite.png#xywh=4,0,4,8", red); }
		#clip_percent { background-image: image("sprite.png#xywh=percent:50,0,50,100"); }
		#clip_edge { background-image: image(url("sprite.png#xywh=pixel:11,0,20,1"), red); }
		#clip_empty { background-image: image("sprite.png#xywh=12,0,1,1", yellow); }
		#clip_repeat { background: image("sprite.png#xywh=4,0,8,8") repeat-x #404040; }
		#mirrored, #override { background-image: image(ltr "sprite.png"); }
		#same { direction: rtl; background-image: image(rtl url(sprite.png)); }
		#neutral { background-image: image(url(sprite.png)); }
		#override { direction: ltr; }
		#nested_set { background-image: image-set(image(rtl "sprite.png#xywh=4,0,8,8", yellow) 1x, url(sprite.png) 2x); }
		#alpha { background-image: image("alpha.png", lime); }
		#overlay { background-image: image(rgb(0 0 255 / 50%)), url(sprite.png); }
		#invalid { background-image: url(sprite.png); background-image: image(ltr "sprite.png", bogus); }
		#inherited { background-image: image(ltr "sprite.png"); }
		@supports (background-image: image("fallback.png", currentColor)) {
			#variable { color: lime; background-image: image(var(--asset, "missing.png"), currentColor); }
		}
	</style>
	<div class="probe" id="solid"></div><div class="probe" id="repeat_color"></div>
	<div class="probe" id="missing"></div><div class="probe" id="bad_fragment"></div>
	<div class="probe" id="clip"></div><div class="probe" id="clip_percent"></div>
	<div class="probe" id="clip_edge"></div><div class="probe" id="clip_empty"></div>
	<div class="probe" id="clip_repeat"></div><div class="probe" id="mirrored" dir="RTL"></div>
	<div class="probe" id="same"></div><div class="probe" id="neutral" dir="rtl"></div>
	<div class="probe" id="override" dir="rtl"></div><div class="probe" id="nested_set"></div>
	<div class="probe" id="alpha"></div><div class="probe" id="overlay"></div>
	<div class="probe" id="invalid"></div><div class="probe" id="variable"></div>
	<section dir="rtl"><div class="probe" id="inherited"></div></section>"""

	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(300, 100)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(180.0, 100.0)
	label.document = document
	viewport.add_child(label)
	var imported: HTMLDocument = ResourceLoader.load("{IMAGE_FUNCTION_UID}") as HTMLDocument
	var imported_label: HTMLLabel = HTMLLabel.new()
	imported_label.position = Vector2(190.0, 0.0)
	imported_label.size = Vector2(100.0, 50.0)
	imported_label.document = imported
	viewport.add_child(imported_label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw

	var result: Image = viewport.get_texture().get_image()
	var empty: Color = Color("#404040")
	var samples: Array[Array] = [
		["solid", Vector2(3.0, 2.0), Color.LIME],
		["solid", Vector2(12.0, 7.0), Color.LIME],
		["solid", Vector2(13.0, 7.0), empty],
		["repeat_color", Vector2(26.0, 3.0), Color.BLUE],
		["repeat_color", Vector2(26.0, 7.0), empty],
		["missing", Vector2(28.0, 18.0), Color.LIME],
		["bad_fragment", Vector2(1.0, 1.0), Color.YELLOW],
		["clip", Vector2(3.0, 7.0), Color.LIME],
		["clip", Vector2(4.0, 7.0), empty],
		["clip_percent", Vector2(1.0, 1.0), Color.LIME],
		["clip_percent", Vector2(5.0, 7.0), Color.BLUE],
		["clip_edge", Vector2.ZERO, Color.BLUE],
		["clip_edge", Vector2(1.0, 0.0), empty],
		["clip_empty", Vector2(1.0, 1.0), Color.YELLOW],
		["clip_repeat", Vector2(10.0, 3.0), Color.LIME],
		["clip_repeat", Vector2(14.0, 3.0), Color.BLUE],
		["mirrored", Vector2(1.0, 1.0), Color.BLUE],
		["mirrored", Vector2(10.0, 1.0), Color.RED],
		["same", Vector2(1.0, 1.0), Color.RED],
		["neutral", Vector2(1.0, 1.0), Color.RED],
		["override", Vector2(1.0, 1.0), Color.RED],
		["nested_set", Vector2(1.0, 1.0), Color.BLUE],
		["nested_set", Vector2(7.0, 1.0), Color.LIME],
		["alpha", Vector2(1.0, 1.0), empty],
		["overlay", Vector2(1.0, 1.0), Color("#800080")],
		["invalid", Vector2(1.0, 1.0), Color.RED],
		["variable", Vector2(1.0, 1.0), Color.LIME],
		["inherited", Vector2(1.0, 1.0), Color.BLUE],
	]
	for sample: Array in samples:
		var id: String = sample[0]
		var offset: Vector2 = sample[1]
		var color: Color = sample[2]
		_check_pixel(result, _label_rect(label, id).position + offset, color, "Annotated image for %s" % id)
	_check(imported != null, "Load the imported image() document by UID")
	if imported != null:
		var textures: Dictionary = imported.get("_textures")
		_check(textures.has("res://sampling.svg"), "Import a bare image() path with xywh as an image dependency")
		var top_left: Vector2 = imported_label.position + _label_rect(imported_label, "imported_image").position
		_check_pixel(result, top_left, Color.LIME, "The embedded stylesheet paints the cropped asset after import")
	var layout: HTMLLayout = document.create_layout()
	layout.css_diagnostics_enabled = true
	_check(layout.layout(Vector2i(180, 100)) == OK, "Lay out the image() diagnostics fixture")
	_check(layout.get_element_by_id("invalid").get_css_diagnostics().size() == 1, "Report an invalid image() fallback color")

	var direction_override: HTMLElement = label.get_element_by_id("override")
	_check(direction_override.set_attribute("style", "direction: revert") == OK, "Revert the explicit CSS direction")
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	_check_pixel(viewport.get_texture().get_image(), _label_rect(label, "override").position + Vector2.ONE, Color.BLUE,
		"Direction rollback restores the HTML hint and flips the image")
	viewport.queue_free()


func _check_cross_fade() -> void:
	var red: Image = Image.create_empty(8, 4, false, Image.FORMAT_RGBA8)
	red.fill(Color.RED)
	var blue: Image = Image.create_empty(24, 12, false, Image.FORMAT_RGBA8)
	blue.fill(Color.BLUE)
	var stripe: Image = Image.create_empty(12, 4, false, Image.FORMAT_RGBA8)
	stripe.fill_rect(Rect2i(0, 0, 4, 4), Color.RED)
	stripe.fill_rect(Rect2i(4, 0, 4, 4), Color.LIME)
	stripe.fill_rect(Rect2i(8, 0, 4, 4), Color.BLUE)
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_textures", {
		"res://mix-red.png": ImageTexture.create_from_image(red),
		"res://mix-blue.png": ImageTexture.create_from_image(blue),
		"res://mix-stripe.png": ImageTexture.create_from_image(stripe),
	})
	document.text = """<!doctype html><style>
		body { margin: 0; display: grid; grid-template-columns: repeat(5, 36px); grid-auto-rows: 22px; }
		div { width: 30px; height: 19px; background-color: black; background-repeat: no-repeat; }
		#omitted { background-image: cross-fade(red 25%, 25% lime, blue); }
		#overflow { background-image: cross-fade(red 80%, blue 80%, lime); }
		#leftover { background-color: white; background-image: cross-fade(red 25%); }
		#premul { background-image: cross-fade(red 40%, rgb(0 255 0 / 50%) 20%, transparent 40%); }
		#natural { background-image: cross-fade(url(mix-red.png) 75%, url(mix-blue.png) 25%); }
		#tint { background-image: cross-fade(white 80%, url(mix-red.png) 20%); }
		#linear { background: cross-fade(linear-gradient(to right, red 50%, blue 50%), lime)
			no-repeat top left / 20px 10px black; }
		#radial { background: cross-fade(radial-gradient(circle closest-side, red 30%, blue 30%), lime)
			no-repeat top left / 20px 10px black; }
		#conic { background: cross-fade(conic-gradient(red 50%, blue 50%), lime)
			no-repeat top left / 20px 10px black; }
		#nested { background-image: cross-fade(red 20%, cross-fade(20% lime, 80% blue) 50%, transparent 30%); }
		#set { background-image: cross-fade(image-set("mix-red.png" 1x, "mix-blue.png" 2x), lime); }
		#set_outer { background-image: image-set(cross-fade(image("missing.png", blue), lime) 1x, url(mix-red.png) 2x); }
		#tiled { background: cross-fade(image(ltr "mix-stripe.png#xywh=4,0,8,4"), red) repeat-x black;
			direction: rtl; image-rendering: crisp-edges; }
		#invalid { background-image: image(lime); background-image: cross-fade(red 101%, blue); }
		#missing { background-image: cross-fade(url(missing.png) 50%, blue 50%); }
		#zero { background-image: cross-fade(url(mix-blue.png) 0%, blue 0%); }
		#nested_size { background-image: cross-fade(cross-fade(url(mix-red.png) 25%, white 75%), url(mix-blue.png)); }
		@supports (background-image: cross-fade(red, blue)) {
			#math { --weight: calc(10% + 15%); color: lime; background-image: cross-fade(currentColor var(--weight), blue); }
		}
	</style>"""
	for id: String in ["omitted", "overflow", "leftover", "premul", "natural", "tint", "linear", "radial", "conic",
		"nested", "set", "set_outer", "tiled", "invalid", "missing", "zero", "nested_size", "math",]:
		document.text += '<div id="%s"></div>' % id

	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(180, 90)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(180.0, 90.0)
	label.document = document
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var output: Image = viewport.get_texture().get_image()
	var samples: Array[Array] = [
		["omitted", Vector2.ONE, Color("#404080")],
		["overflow", Vector2.ONE, Color("#800080")],
		["leftover", Vector2.ONE, Color("#ffbfbf")],
		["premul", Vector2.ONE, Color("#661a00")],
		["natural", Vector2(11.0, 5.0), Color("#bf0040")],
		["natural", Vector2(12.0, 5.0), Color.BLACK],
		["natural", Vector2(11.0, 6.0), Color.BLACK],
		["tint", Vector2(7.0, 3.0), Color("#ffcccc")],
		["tint", Vector2(8.0, 0.0), Color.BLACK],
		["linear", Vector2(3.0, 4.0), Color("#808000")],
		["linear", Vector2(15.0, 4.0), Color("#008080")],
		["radial", Vector2(10.0, 4.0), Color("#808000")],
		["radial", Vector2.ONE, Color("#008080")],
		["conic", Vector2(15.0, 2.0), Color("#808000")],
		["conic", Vector2(4.0, 2.0), Color("#008080")],
		["nested", Vector2.ONE, Color("#331a66")],
		["set", Vector2.ONE, Color("#808000")],
		["set_outer", Vector2.ONE, Color("#008080")],
		["tiled", Vector2(9.0, 2.0), Color("#800080")],
		["tiled", Vector2(13.0, 2.0), Color("#808000")],
		["tiled", Vector2(13.0, 4.0), Color.BLACK],
		["invalid", Vector2.ONE, Color.LIME],
		["missing", Vector2.ONE, Color("#000080")],
		["zero", Vector2.ONE, Color.BLACK],
		["nested_size", Vector2(20.0, 9.0), Color("#8060df")],
		["nested_size", Vector2(21.0, 9.0), Color.BLACK],
		["math", Vector2.ONE, Color("#0040bf")],
	]
	for sample: Array in samples:
		var id: String = sample[0]
		var offset: Vector2 = sample[1]
		var color: Color = sample[2]
		_check_pixel(output, _label_rect(label, id).position + offset, color, "Cross-fade for %s" % id)
	var diagnostics: HTMLLayout = document.create_layout()
	diagnostics.css_diagnostics_enabled = true
	_check(diagnostics.layout(Vector2i(180, 90)) == OK, "Lay out the cross-fade diagnostics fixture")
	_check(diagnostics.get_element_by_id("invalid").get_css_diagnostics().size() == 1,
		"Reject a cross-fade weight outside the percentage range")
	viewport.queue_free()


func _check_image_resolution() -> void:
	var generated: Image = Image.create_empty(48, 24, false, Image.FORMAT_RGBA8)
	generated.fill(Color.RED)
	var texture: ImageTexture = ImageTexture.create_from_image(generated)
	if not _write_image_dpi_fixtures():
		_check(false, "Write JPEG and WebP metadata fixtures")
		return
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_textures", {
		"res://density-default.png": texture,
		"res://density-jfif.jpg": texture,
		"res://density-exif.jpg": texture,
		"res://density-exif.webp": texture,
		"res://density-late.webp": texture,
		"res://density-small.png": ImageTexture.create_from_image(generated.get_region(Rect2i(0, 0, 24, 12))),
		"res://density-vector.svg": texture,
	})
	document.text = """<!doctype html><style>
		body { margin: 0; display: grid; grid-template-columns: repeat(5, 72px); grid-auto-rows: 36px;
			align-items: start; justify-items: start; }
		img { display: block; }
		.resolution_parent { image-resolution: 192dpi; }
		.tile { width: 60px; height: 28px; background-color: #303030; background-repeat: no-repeat; }
		#cascade { image-resolution: 2x; image-resolution: snap; }
		#fit { width: 32px; height: 24px; object-fit: none; background-color: #303030; image-resolution: 2x; }
		#background { background-image: url(density-live.png); image-resolution: from-image; }
		#set_density { background-image: image-set(url(density-live.png) 3x); }
		#set_metadata { background-image: image-set(url(density-live.png)); }
		#set_override { background-image: image-set(url(density-live.png) 4x); image-resolution: 2x; }
		#set_preferred { background-image: image-set(url(density-live.png) 4x); image-resolution: from-image 2x; }
		#blend { background-image: cross-fade(white, url(density-live.png)); image-resolution: from-image; }
		#crop { background-image: image("density-live.png#xywh=0,0,24,12"); image-resolution: from-image; }
		@supports (image-resolution: from-image calc(96dpi + 1dppx) snap) {
			#dynamic { --density: calc(96dpi + 1dppx); image-resolution: var(--density) snap; }
		}
	</style>
	<img id="default" src="density-live.png">
	<div class="resolution_parent"><img id="inherited" src="density-default.png"></div>
	<img id="metadata" src="density-live.png" style="image-resolution: from-image 3x">
	<img id="fallback" src="density-default.png" style="image-resolution: from-image 3x">
	<img id="invalid_metadata" src="density-asymmetric.png" style="image-resolution: from-image 3x">
	<img id="override" src="density-live.png" style="image-resolution: 3x">
	<img id="snap" src="density-default.png" style="image-resolution: 260dpi snap">
	<img id="snap_low" src="density-default.png" style="image-resolution: snap 0.75x">
	<img id="fractional" src="density-default.png" style="image-resolution: 0.75x">
	<img id="vector" src="density-vector.svg" style="image-resolution: 3x">
	<img id="cascade" src="density-default.png">
	<img id="dynamic" src="density-default.png">
	<img id="jfif" src="density-jfif.jpg" style="image-resolution: from-image 2x">
	<img id="exif_jpeg" src="density-exif.jpg" style="image-resolution: from-image 2x">
	<img id="exif_webp" src="density-exif.webp" style="image-resolution: from-image 2x">
	<img id="fit" src="density-live.png">
	<div id="background" class="tile"></div><div id="set_density" class="tile"></div>
	<div id="set_metadata" class="tile"></div><div id="set_override" class="tile"></div>
	<div id="set_preferred" class="tile"></div><div id="blend" class="tile"></div><div id="crop" class="tile"></div>
	<img id="snap_exact" src="density-small.png" style="image-resolution: snap 0.5x">
	<img id="late_metadata" src="density-late.webp" style="image-resolution: from-image 2x">"""
	var layout: HTMLLayout = document.create_layout()
	layout.css_diagnostics_enabled = true
	_check(layout.layout(Vector2i(360, 182)) == OK, "Lay out the image resolution fixture")
	var sizes: Dictionary[String, Vector2] = {
		"default": Vector2(48.0, 24.0),
		"inherited": Vector2(24.0, 12.0),
		"metadata": Vector2(24.0, 12.0),
		"fallback": Vector2(16.0, 8.0),
		"invalid_metadata": Vector2(16.0, 8.0),
		"override": Vector2(16.0, 8.0),
		"snap": Vector2(16.0, 8.0),
		"snap_low": Vector2(48.0, 24.0),
		"fractional": Vector2(64.0, 32.0),
		"vector": Vector2(48.0, 24.0),
		"cascade": Vector2(24.0, 12.0),
		"dynamic": Vector2(24.0, 12.0),
		"jfif": Vector2(16.0, 8.0),
		"exif_jpeg": Vector2(12.0, 6.0),
		"exif_webp": Vector2(12.0, 6.0),
		"snap_exact": Vector2(48.0, 24.0),
		"late_metadata": Vector2(24.0, 12.0),
	}
	for id: String in sizes:
		_check(_rect(layout, id).size.distance_to(sizes[id]) < 0.01,
			"Natural image resolution for %s is %s, expected %s" % [id, _rect(layout, id).size, sizes[id]])
	_check(_has_diagnostic(layout, "image-resolution", "snap", "style_element"), "Reject snap without a resolution source")
	var imported: HTMLDocument = ResourceLoader.load("{IMAGE_RESOLUTION_UID}") as HTMLDocument
	_check(imported != null and not FileAccess.file_exists("res://density-import.png"),
		"Load the imported density fixture without the original PNG")
	if imported != null:
		var saved: Dictionary = imported.get("_image_dpi")
		var dpi: float = float(saved.get("res://density-import.png", 0.0))
		_check(absf(dpi - 192.0) < 0.01, "The HTML importer preserves the image density")
		var imported_layout: HTMLLayout = imported.create_layout()
		_check(imported_layout.layout(Vector2i(100, 80)) == OK and
			_rect(imported_layout, "imported_density").size.distance_to(Vector2(24.0, 12.0)) < 0.01,
			"An imported image still uses its density after removal of the original PNG")

	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(360, 182)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(360.0, 182.0)
	label.document = document
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var output: Image = viewport.get_texture().get_image()
	var fit: Vector2 = _label_rect(label, "fit").position
	_check_pixel(output, fit + Vector2(3.0, 8.0), Color("#303030"), "Object-fit none uses the density-adjusted image width")
	_check_pixel(output, fit + Vector2(4.0, 8.0), Color.RED, "The smaller object is centered in its layout box")
	var tile_widths: Dictionary[String, int] = {
		"background": 24, "set_density": 16, "set_metadata": 24, "set_override": 24,
		"set_preferred": 12, "blend": 24, "crop": 12,
	}
	for id: String in tile_widths:
		var origin: Vector2 = _label_rect(label, id).position
		var expected: Color = Color("#ff8080") if id == "blend" else Color.RED
		_check_pixel(output, origin + Vector2(float(tile_widths[id] - 1), 1.0), expected, "Density-adjusted tile for %s" % id)
		_check_pixel(output, origin + Vector2(float(tile_widths[id]), 1.0), Color("#303030"), "Tile ends at the CSS width for %s" % id)
	viewport.queue_free()


func _check_image_orientation() -> void:
	var markup: String = """<!doctype html><style>
		body { margin: 0; display: grid; grid-template-columns: repeat(5, 52px); grid-auto-rows: 42px;
			align-items: start; justify-items: start; }
		img { display: block; }
		.tile { width: 42px; height: 39px; background: #303030 no-repeat; }
		#invalid { image-orientation: none; image-orientation: 90deg flip flip; }
		#fit { width: 32px; height: 34px; object-fit: none; background: #303030; }
		#crop { background-image: image("orientation-6.png#xywh=0,0,10,16"); }
		#decorative { background-image: image-set(url("orientation-6.png") 1x); image-orientation: 180deg; }
		#decorative_none { background-image: cross-fade(white, url("orientation-6.png")); image-orientation: none; }
		#generated::before { content: url("orientation-6.png"); image-orientation: none; }
	</style>"""
	for orientation: int in range(1, 9):
		markup += '<img id="exif%d" src="orientation-%d.png">' % [orientation, orientation]
	markup += """
	<div style="image-orientation: none"><img id="inherited" src="orientation-6.png"></div>
	<img id="invalid" src="orientation-6.png">
	<img id="manual" src="orientation-6.png" style="--angle: calc(50grad + 0turn); image-orientation: flip var(--angle)">
	<img id="round_up" src="orientation-6.png" style="image-orientation: -45deg">
	<img id="manual_flip" src="orientation-6.png" style="image-orientation: flip">
	<img id="density" src="orientation-6.png" style="image-resolution: from-image">
	<img id="late" src="orientation-late.png">
	<img id="fit" src="orientation-6.png">
	<div id="crop" class="tile"></div>
	<div id="decorative" class="tile"></div>
	<div id="decorative_none" class="tile"></div>
	<div id="generated"></div>"""
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	_check(layout.parse(markup) == OK and layout.layout(Vector2i(260, 210)) == OK, "Lay out image orientation fixtures")
	for orientation: int in range(1, 9):
		var extent: Vector2 = Vector2(32.0, 20.0) if orientation <= 4 else Vector2(20.0, 32.0)
		_check(_rect(layout, "exif%d" % orientation).size.is_equal_approx(extent),
			"EXIF orientation %d uses the oriented natural size" % orientation)
	for id: String in ["inherited", "invalid", "round_up", "manual_flip", "late", "generated"]:
		_check(is_equal_approx(_rect(layout, id).size.x, 32.0), "%s uses the encoded width" % id)
	_check(_rect(layout, "density").size.distance_to(Vector2(10.0, 16.0)) < 0.01,
		"Density and orientation both apply when pHYs precedes eXIf")
	_check(_has_diagnostic(layout, "image-orientation", "90deg flip flip", "style_element"),
		"An invalid image-orientation declaration preserves the earlier winner")

	var imported: HTMLDocument = ResourceLoader.load("{IMAGE_ORIENTATION_UID}") as HTMLDocument
	_check(imported != null and not FileAccess.file_exists("res://orientation-import.png"),
		"Load the imported orientation fixture without the source PNG")
	if imported != null:
		var imported_layout: HTMLLayout = imported.create_layout()
		_check(imported_layout.layout(Vector2i(100, 80)) == OK and
			_rect(imported_layout, "imported_orientation").size.is_equal_approx(Vector2(20.0, 32.0)),
			"Imported HTML retains image orientation alongside its texture dependency")

	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(260, 210)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(260.0, 210.0)
	label.text = markup
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var output: Image = viewport.get_texture().get_image()
	var corners: Array[String] = [
		"rgby", "gryb", "ybgr", "byrg", "rbgy", "bryg", "ygbr", "gyrb",
	]
	var palette: Dictionary[String, Color] = {
		"r": Color.RED,
		"g": Color.GREEN,
		"b": Color.BLUE,
		"y": Color.YELLOW,
	}
	for orientation: int in range(1, 10):
		var id: String = "exif%d" % orientation if orientation <= 8 else "manual"
		var box: Rect2 = _label_rect(label, id)
		var corner_order: String = corners[orientation - 1] if orientation <= 8 else corners[4]
		for corner: int in range(4):
			var x: float = 3.0 if corner % 2 == 0 else box.size.x - 4.0
			var y: float = 3.0 if corner < 2 else box.size.y - 4.0
			_check_pixel(output, box.position + Vector2(x, y), palette[corner_order[corner]],
				"%s paints the oriented corner %d" % [id, corner])
	var fit: Vector2 = _label_rect(label, "fit").position
	_check_pixel(output, fit + Vector2(5.0, 3.0), Color("#303030"), "Object-fit uses the rotated width")
	_check_pixel(output, fit + Vector2(8.0, 3.0), Color.BLUE, "Object-fit centers the oriented pixels")
	var crop: Vector2 = _label_rect(label, "crop").position
	_check_pixel(output, crop + Vector2(4.0, 4.0), Color.BLUE, "Image fragments are cropped after orientation")
	_check_pixel(output, crop + Vector2(10.0, 4.0), Color("#303030"), "The cropped tile keeps the fragment dimensions")
	var decorative: Vector2 = _label_rect(label, "decorative").position
	_check_pixel(output, decorative + Vector2(4.0, 4.0), Color.BLUE, "Decorative image-set uses EXIF despite a legacy angle")
	var blend: Vector2 = _label_rect(label, "decorative_none").position
	_check_pixel(output, blend + Vector2(4.0, 4.0), Color("#ff8080"), "Cross-fade uses the original pixels with image-orientation none")
	_check_pixel(output, blend + Vector2(24.0, 3.0), Color("#80ff80"), "Cross-fade sizes its tile from the unrotated source")
	viewport.queue_free()
	_check_encoded_image_orientation()


func _check_encoded_image_orientation() -> void:
	var pixels: Image = Image.create_empty(32, 20, false, Image.FORMAT_RGB8)
	pixels.fill(Color.RED)
	var tiff: PackedByteArray = [0x49, 0x49, 42, 0, 8, 0, 0, 0, 1, 0,
		0x12, 0x01, 3, 0, 1, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0,]
	var jpeg_source: PackedByteArray = pixels.save_jpg_to_buffer()
	var jpeg: PackedByteArray = [0xff, 0xd8, 0xff, 0xe1, 0, 34, 69, 120, 105, 102, 0, 0,]
	jpeg.append_array(tiff)
	jpeg.append_array(jpeg_source.slice(2))

	var webp_source: PackedByteArray = pixels.save_webp_to_buffer()
	var webp: PackedByteArray = webp_source.slice(0, 12)
	var vp8x: PackedByteArray = [86, 80, 56, 88, 10, 0, 0, 0, 8, 0, 0, 0, 31, 0, 0, 19, 0, 0,]
	var exif: PackedByteArray = [69, 88, 73, 70, 26, 0, 0, 0,]
	webp.append_array(vp8x)
	webp.append_array(exif)
	webp.append_array(tiff)
	webp.append_array(webp_source.slice(12))
	webp.encode_u32(4, webp.size() - 8)

	var fixtures: Dictionary[String, PackedByteArray] = {
		"orientation.jpg": jpeg,
		"orientation.webp": webp,
	}
	var textures: Dictionary[String, Texture2D] = {}
	var markup: String = '<!doctype html><style>img { display: block; }</style>'
	for name: String in fixtures:
		var fixture_path: String = "res://" + name
		var file: FileAccess = FileAccess.open(fixture_path, FileAccess.WRITE)
		var decoded: Image = Image.new()
		var error: Error = decoded.load_jpg_from_buffer(fixtures[name]) if name.ends_with(".jpg") else decoded.load_webp_from_buffer(fixtures[name])
		if file == null or not file.store_buffer(fixtures[name]) or error != OK:
			_check(false, "Read and decode the EXIF fixture for %s" % name)
			return
		file.close()
		_check(decoded.get_size() == Vector2i(32, 20), "Godot decodes %s in its encoded orientation" % name)
		textures[fixture_path] = ImageTexture.create_from_image(decoded)
		markup += '<img id="%s" src="%s">' % [name, name]
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_textures", textures)
	document.text = markup
	var layout: HTMLLayout = document.create_layout()
	_check(layout.layout(Vector2i(100, 100)) == OK, "Lay out real JPEG and WebP EXIF images")
	for name: String in fixtures:
		_check(_rect(layout, name).size.is_equal_approx(Vector2(20.0, 32.0)),
			"Image orientation uses the EXIF tag in %s" % name)


func _check_border_image() -> void:
	var source: String = """<!doctype html><style>
		body { margin: 0; background: #303030; display: grid; grid-template-columns: repeat(5, 72px);
			grid-auto-rows: 60px; justify-items: start; align-items: start; }
		.box { width: 26px; height: 24px; margin: 6px; border: 4px solid #008855;
			background-color: #303030; image-rendering: crisp-edges; }
		#stretch { border-image: url(border-image.png) 4; }
		#fill { border-image: fill calc(100% / 3) / 2 / 2px round stretch url(border-image.png); border-radius: 9px; }
		#repeat { border-image: url(border-image.png) 4 repeat stretch; }
		#space { border-image: url(border-image.png) 4 / 8px space stretch; }
		#round { border-image: url(border-image.png) 4 / 8px round; }
		#no_room { border-image: url(border-image.png) 4 / 15px space; }
		#fallback { border-image: url(missing-border-image.png) 4; }
		#reset { border-image: url(border-image.png) 4 fill; border: 4px solid #008855; }
		#variable { --frame: url(border-image.png) 4 fill / auto; border-image: var(--frame);
			border-image-slice: bogus; border-image-width: calc(4px + 2%) 4px; }
		#outset_only { border-image: url(border-image.png) 4 // 2; }
		#oriented { border-image: url(orientation-6.png) 50% / 4px; }
		#generated { --source: linear-gradient(red, red); border-image: var(--source) 25% / 4px; }
		#absolute_width { border: none; border-image: url(border-image.png) 4 / 4px; }
		#translated { border-image: url(border-image.png) 4; transform: translate(10px, 4px); }
		.wrap { width: 70px; margin: 8px; font: 14px monospace; line-height: 24px; }
		#longhands { border-image-source: url(border-image.png); border-image-slice: 4 fill;
			border-image-width: 4px; border-image-outset: 3px; border-image-repeat: stretch; border-top: 4px dashed blue; }
		#hires { image-resolution: 2x; border-image: url(border-image.png) 4 / auto; }
		#clip_frame { width: 32px; height: 32px; margin: 6px; overflow: hidden; border-radius: 12px; }
		#clipped { width: 24px; height: 24px; border: 4px solid black;
			border-image: url(border-image.png) 4 fill / 1 / 3px; image-rendering: crisp-edges; }
		#wrapped { border: 4px solid transparent; border-image: url(border-image.png) 4 stretch;
			image-rendering: crisp-edges; color: transparent; }
	</style>
	<div class="box" id="stretch"></div><div class="box" id="fill"></div>
	<div class="box" id="repeat"></div><div class="box" id="space"></div><div class="box" id="round"></div>
	<div class="box" id="fallback"></div><div class="box" id="reset"></div><div class="box" id="variable"></div>
	<div class="box" id="outset_only"></div><div class="box" id="oriented"></div><div class="box" id="generated"></div>
	<div class="box" id="absolute_width"></div><div class="box" id="translated"></div><div class="box" id="no_room"></div>
	<div class="wrap"><span id="wrapped">AAAAAA<br>BBBBBB</span></div>
	<div class="box" id="longhands"></div><div class="box" id="hires"></div>
	<div id="clip_frame"><div id="clipped"></div></div>"""
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	_check(layout.parse(source) == OK and layout.layout(Vector2i(360, 240)) == OK, "Lay out border image fixture")
	_check(_rect(layout, "fill").size.is_equal_approx(_rect(layout, "stretch").size),
		"Border image width and outset preserve CSS box geometry")
	var fill: Rect2 = _rect(layout, "fill")
	var overhang_hit: HTMLElement = layout.get_element_at(fill.position - Vector2(1.0, 1.0))
	_check(overhang_hit == null or overhang_hit.get_attribute("id") != "fill", "Border image outset does not receive hits")
	_check(_has_diagnostic(layout, "border-image-slice", "bogus", "style_element"), "Invalid slices use CSS diagnostics")

	var imported: HTMLDocument = ResourceLoader.load("{BORDER_IMAGE_UID}") as HTMLDocument
	_check(imported != null and not FileAccess.file_exists("res://border-import.png"),
		"Load the border image resource without the original CSS and PNG")
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(420, 240)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(360.0, 240.0)
	label.text = source
	viewport.add_child(label)
	var imported_label: HTMLLabel = HTMLLabel.new()
	imported_label.position = Vector2(370.0, 0.0)
	imported_label.size = Vector2(40.0, 40.0)
	imported_label.document = imported
	viewport.add_child(imported_label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var output: Image = viewport.get_texture().get_image()
	var samples: Array[Array] = [
		["stretch", Vector2(1.0, 1.0), Color.RED, "Top-left corner"],
		["stretch", Vector2(33.0, 1.0), Color.BLUE, "Top-right corner"],
		["stretch", Vector2(1.0, 30.0), Color.YELLOW, "Bottom-left corner"],
		["stretch", Vector2(33.0, 30.0), Color.CYAN, "Bottom-right corner"],
		["stretch", Vector2(16.0, 16.0), Color("#303030"), "Center is omitted without fill"],
		["stretch", Vector2(28.0, 1.0), Color.WHITE, "Stretch preserves the full source edge"],
		["fill", Vector2(16.0, 16.0), Color("#ee8200"), "Fill paints over the CSS background"],
		["fill", Vector2(-1.0, -1.0), Color.RED, "Outset paints beyond even a rounded border box"],
		["repeat", Vector2(4.0, 1.0), Color.MAGENTA, "Repeat centers a tile and crops the first repetition"],
		["repeat", Vector2(5.0, 1.0), Color.WHITE, "Repeat reaches the second region of a partial tile"],
		["repeat", Vector2(7.0, 1.0), Color.MAGENTA, "A new repeat starts at the centered phase"],
		["space", Vector2(8.0, 1.0), Color("#303030"), "Space inserts a leading gap"],
		["space", Vector2(9.0, 1.0), Color.MAGENTA, "Space keeps a whole tile"],
		["space", Vector2(25.0, 1.0), Color("#303030"), "Space gives the trailing gap the same width"],
		["no_room", Vector2(16.0, 1.0), Color("#303030"), "Space drops partial side tiles"],
		["round", Vector2(16.0, 1.0), Color.WHITE, "Round scales the first tile to half the side"],
		["round", Vector2(17.0, 1.0), Color.MAGENTA, "Round starts the second tile with no gap"],
		["fallback", Vector2(1.0, 1.0), Color("#008855"), "Missing source uses the ordinary border"],
		["reset", Vector2(1.0, 1.0), Color("#008855"), "The border shorthand resets border-image"],
		["variable", Vector2(16.0, 16.0), Color("#ee8200"), "Variable shorthand and longhand both cascade"],
		["outset_only", Vector2(-6.0, -5.0), Color.RED, "Double slash and border-width multipliers"],
		["oriented", Vector2(1.0, 1.0), Color.BLUE, "Nine-slice uses the oriented source"],
		["generated", Vector2(1.0, 1.0), Color.RED, "Gradient border uses a generated image source"],
		["absolute_width", Vector2(1.0, 1.0), Color.RED, "Absolute image width can paint with no ordinary border"],
		["translated", Vector2(1.0, 1.0), Color.RED, "Border image follows the element transform"],
		["longhands", Vector2(-1.0, -1.0), Color.RED, "Longhands survive a single-side border declaration"],
		["longhands", Vector2(16.0, 16.0), Color("#ee8200"), "Independent longhands paint the center"],
		["hires", Vector2(1.0, 1.0), Color.RED, "Auto width uses the natural density-adjusted slice size"],
		["hires", Vector2(2.0, 2.0), Color("#303030"), "A density of two halves the four-pixel auto border width"],
		["clipped", Vector2(0.0, 0.0), Color("#303030"), "An ancestor's rounded clip masks the outset image"],
		["clipped", Vector2(16.0, 16.0), Color("#ee8200"), "The filled center remains inside the ancestor clip"],
	]
	for sample: Array in samples:
		var id: String = sample[0]
		var offset: Vector2 = sample[1]
		var color: Color = sample[2]
		var description: String = sample[3]
		_check_pixel(output, _label_rect(label, id).position + offset, color, description)
	var inline_element: HTMLElement = label.get_element_by_id("wrapped")
	var fragments: Array[Rect2] = inline_element.get_client_rects()
	_check(fragments.size() == 2, "The bordered inline wraps into two fragments")
	if fragments.size() == 2:
		_check_pixel(output, fragments[0].position + Vector2(1.0, 1.0), Color.RED, "The first inline fragment has a corner")
		_check_pixel(output, fragments[0].position + Vector2(fragments[0].size.x - 3.0, 1.0), Color.MAGENTA,
			"The first inline fragment uses the first half of the continuous edge")
		_check_pixel(output, fragments[1].position + Vector2(3.0, 1.0), Color.WHITE,
			"The second inline fragment continues the edge without restarting it")
	if imported != null:
		_check_pixel(output, imported_label.position + Vector2(1.0, 1.0), Color.RED,
			"An imported stylesheet's border image paints from the retained texture")
	viewport.queue_free()


func _check_outlines() -> void:
	var source: String = """<!doctype html><style>
		body { margin: 0; background: #303030; display: grid; grid-template-columns: repeat(5, 80px);
			grid-auto-rows: 64px; justify-items: start; align-items: start; }
		.box { width: 32px; height: 24px; margin: 12px; background: blue; }
		#positive { outline-offset: calc(2px + 2px); color: yellow; outline: solid calc(1px + 2px); overflow: hidden; }
		#negative { position: relative; outline: 3px solid red; outline-offset: -6px; }
		#negative > div { position: absolute; inset: 0; background: blue; }
		#negative_limit { outline: 3px solid lime; outline-offset: -1000px; }
		#rounded { border-radius: 8px; outline: 2px solid magenta; outline-offset: 2px; }
		#double { --outline: 6px double cyan; outline: var(--outline); outline-offset: 2px; }
		#dashed { outline: 2px dashed yellow; }
		#auto { outline: 9px auto lime; }
		#accent { outline: auto; }
		#invalid { outline: 2px solid red; outline: 9px auto auto auto; outline-style: hidden; outline-offset: 7%; }
		#transformed { outline: 2px solid yellow; transform: translate(8px, 4px) scale(1.5); }
		#wrap { margin: 10px; width: 58px; font: 14px monospace; line-height: 22px; }
		#inline_link { outline: 2px solid red; text-decoration: none; color: white; }
		#clip { width: 32px; height: 28px; margin: 12px; overflow: hidden; border-radius: 9px; }
		#clipped { width: 26px; height: 20px; margin: 2px; background: blue; outline: 3px solid red; }
		#focus { margin: 12px; background: blue; box-shadow: none; }
		#focus:focus-visible { outline: 2px solid white; outline-offset: 2px; box-shadow: none; }
		#parent { outline-width: thick; outline-style: solid; outline-color: currentColor; color: yellow; outline-offset: -2px; }
		#inherits { width: 14px; height: 12px; position: relative; left: 8px; top: 6px; color: lime;
			outline-width: inherit; outline-style: inherit; outline-color: inherit; outline-offset: inherit; }
	</style>
	<div class="box" id="positive"></div><div class="box" id="negative"><div></div></div>
	<div class="box" id="negative_limit"></div><div class="box" id="rounded"></div><div class="box" id="double"></div>
	<div class="box" id="dashed"></div><div class="box" id="auto"></div><div class="box" id="accent"></div>
	<div class="box" id="invalid"></div><div class="box" id="transformed"></div>
	<div id="wrap"><a id="inline_link" href="#positive">ABCD<br>EFGH</a></div>
	<div id="clip"><div id="clipped"></div></div>
	<button id="focus" type="button">Focus</button>
	<div class="box" id="parent"><div id="inherits"></div></div>"""
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	_check(layout.parse(source) == OK and layout.layout(Vector2i(400, 192)) == OK, "Lay out CSS outlines")
	var positive: HTMLElement = layout.get_element_by_id("positive")
	var positive_box: Rect2 = positive.get_bounding_rect()
	var content_size: Vector2i = layout.get_content_size()
	_check(positive_box.size.is_equal_approx(Vector2(32.0, 24.0)), "Outline width never contributes to box geometry")
	var hit: HTMLElement = layout.get_element_at(positive_box.position + Vector2(8.0, -6.0))
	_check(hit == null or hit.get_attribute("id") != "positive", "Outline ink is outside the hit area")
	_check(positive.set_attribute("style", "outline-offset: 1000px") == OK and layout.layout(Vector2i(400, 192)) == OK,
		"Update the outline offset")
	_check(layout.get_content_size() == content_size, "A large outline does not expand the scrollable range")
	_check(_has_diagnostic(layout, "outline-style", "hidden", "style_element") and
		_has_diagnostic(layout, "outline-offset", "7%", "style_element") and
		_has_diagnostic(layout, "outline", "9px auto auto auto", "style_element"), "Invalid outlines use CSS diagnostics")

	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(400, 192)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(400.0, 192.0)
	label.text = source
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var output: Image = viewport.get_texture().get_image()
	var samples: Array[Array] = [
		["positive", Vector2(12.0, -6.0), Color.YELLOW, "Positive offset, math width and currentColor"],
		["positive", Vector2(12.0, -2.0), Color("#303030"), "Positive offset leaves a gap"],
		["negative", Vector2(12.0, 2.0), Color.BLUE, "Negative offset leaves the outside clear"],
		["negative", Vector2(12.0, 3.0), Color.RED, "Negative outline paints over positioned descendants"],
		["negative", Vector2(12.0, 6.0), Color.BLUE, "The inside of the negative outline stays clear"],
		["negative_limit", Vector2(16.0, 12.0), Color.LIME, "Large negative offsets keep a visible shape"],
		["negative_limit", Vector2(12.0, 12.0), Color.BLUE, "Large negative offsets clamp around the center"],
		["rounded", Vector2(-4.0, -4.0), Color("#303030"), "The rounded corner cuts away the outset corner"],
		["rounded", Vector2(16.0, -3.0), Color.MAGENTA, "The rounded outline preserves its width along straight edges"],
		["double", Vector2(16.0, -8.0), Color.CYAN, "Double outline outer stroke"],
		["double", Vector2(16.0, -6.0), Color("#303030"), "Double outline gap"],
		["double", Vector2(16.0, -4.0), Color.CYAN, "Double outline inner stroke"],
		["auto", Vector2(16.0, -2.0), Color.LIME, "Auto pattern uses a solid focus outline with author color"],
		["auto", Vector2(16.0, -3.0), Color("#303030"), "Auto pattern uses its own thickness"],
		["invalid", Vector2(16.0, -1.0), Color.RED, "Invalid shorthand and longhands preserve the earlier value"],
		["transformed", Vector2(24.0, -2.0), Color.YELLOW, "Outline follows the visual transform"],
		["clipped", Vector2(-1.0, -1.0), Color("#303030"), "Ancestor rounded overflow clips the outline"],
		["clipped", Vector2(10.0, -1.0), Color.RED, "The rest of the outline remains visible inside the clip"],
		["inherits", Vector2(7.0, 0.0), Color.LIME, "Inherited outline color resolves against the new currentColor"],
	]
	for sample: Array in samples:
		var id: String = sample[0]
		var offset: Vector2 = sample[1]
		var color: Color = sample[2]
		var description: String = sample[3]
		_check_pixel(output, _label_rect(label, id).position + offset, color, description)
	var accent: Color = DisplayServer.get_accent_color()
	if is_zero_approx(accent.a):
		accent = Color("#2563eb")
	_check_pixel(output, _label_rect(label, "accent").position + Vector2(16.0, -1.0), accent,
		"Lone auto uses the display's accent color or DocRender's fallback")

	var dash: Vector2 = _label_rect(label, "dashed").position
	var yellow_pixels: int
	for x: int in range(32):
		if output.get_pixelv(Vector2i(dash) + Vector2i(x, -1)).is_equal_approx(Color.YELLOW):
			yellow_pixels += 1
	_check(yellow_pixels >= 4 and yellow_pixels <= 29, "Dashes alternate strokes and gaps")
	var link: HTMLElement = label.get_element_by_id("inline_link")
	var link_box: Rect2 = link.get_bounding_rect()
	_check(link.get_client_rects().size() == 2, "The outline example wraps into two link fragments")
	_check_pixel(output, link_box.position + Vector2(-1.0, link_box.size.y / 2.0), Color.RED,
		"Wrapped links have a single connected enclosure")

	var button: HTMLElement = label.get_element_by_id("focus")
	var button_box: Rect2 = button.get_bounding_rect()
	var focus_sample: Vector2 = button_box.position + Vector2(button_box.size.x / 2.0, -3.0)
	_check_pixel(output, focus_sample, Color("#303030"), "Focus outline is clear before keyboard input")
	label.grab_focus()
	var tab: InputEventKey = InputEventKey.new()
	tab.keycode = KEY_TAB
	tab.pressed = true
	viewport.push_input(tab)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	_check(button.matches(":focus-visible"), "Tab matches the button's focus-visible rule")
	_check(button.get_bounding_rect().is_equal_approx(button_box), "Focus outlines preserve the button's dimensions")
	_check_pixel(viewport.get_texture().get_image(), focus_sample, Color.WHITE, "Keyboard focus paints the requested outline")
	viewport.queue_free()


func _check_background_tiling() -> void:
	var source: String = """<!doctype html><style>
		body { margin: 0; background: #303030; display: grid; grid-template-columns: repeat(5, 96px);
			grid-auto-rows: 60px; align-items: start; justify-items: start; }
		.tile { width: 44px; height: 44px; background-color: #303030; image-rendering: crisp-edges; }
		#spaced { background: #303030 url(density-live.png) 6px 11px / 8px 8px space no-repeat; }
		#few { width: 13px; background-image: url(density-live.png); background-size: 8px 8px;
			background-position: right 2px; background-repeat: space no-repeat; }
		#oversized { width: 13px; background-image: url(orientation-1.png); background-size: 20px 10px;
			background-position: right top; background-repeat: space no-repeat; }
		#both_space { background-image: url(density-live.png); background-size: 8px 10px; background-repeat: space; }
		#round_x { background-image: url(density-live.png); background-size: calc(8px + 8px) auto;
			background-repeat: round no-repeat; }
		#round_y { height: 43px; background-image: url(density-live.png); background-size: auto 16px;
			background-repeat: no-repeat round; }
		#round_both { height: 42px; background: url(orientation-1.png) left top / 16px 16px round; }
		#round_position { width: 54px; background-image: url(orientation-1.png); background-size: 17px 10px;
			background-position: 25% 0%; background-repeat: round no-repeat; background-repeat: repeat-x space; }
		#gradient { background: linear-gradient(to right, red 50%, blue 50%) left top / 8px 10px space no-repeat; }
		#fallback { background-image: image("missing-tile.png", red); background-size: 8px 8px;
			background-repeat-x: space; background-repeat-y: no-repeat; }
		#blend { background: cross-fade(white, url(density-live.png)) left top / 8px 8px space no-repeat; }
		#layers { background-image: image(red), image(lime), image(blue); background-size: 8px 7px, 10px 7px;
			background-position: 0 0, 0 12px, 0 24px; background-repeat-x: space, no-repeat; background-repeat-y: no-repeat; }
		#padding { background: url(density-live.png) left top / 8px 8px space no-repeat content-box border-box;
			padding: 0 6px; border: 2px solid transparent; }
		#var { --modes: space no-repeat; background-image: image(red); background-size: 8px 10px;
			background-repeat: var(--modes); background-repeat-block: space; background-repeat-x: repeat-y; }
		#alias { background: image(red) 0 0 / 8px 8px repeat-block; }
		#reset { background-repeat-inline: no-repeat; background-repeat-y: no-repeat;
			background: image(red) 0 0 / 8px 8px; }
		#math { background: image(red) calc(100% - 2px) calc(3px + 3px) / calc(4px + 4px) 8px no-repeat; }
		#clipped { border-radius: 10px; background: url(density-live.png) 0 0 / 8px 10px space no-repeat; }
		#flipped { direction: rtl; background: image(ltr "orientation-1.png") 0 0 / 8px 8px space no-repeat; }
		#fractional { background: image(red) 0 0 / calc(100% / 3) 8px space no-repeat; }
		#zero { background: image(red) 0 0 / 0px 10px space; }
	</style>
	<div class="tile" id="spaced"></div><div class="tile" id="few"></div><div class="tile" id="oversized"></div>
	<div class="tile" id="both_space"></div><div class="tile" id="round_x"></div><div class="tile" id="round_y"></div>
	<div class="tile" id="round_both"></div><div class="tile" id="round_position"></div><div class="tile" id="gradient"></div>
	<div class="tile" id="fallback"></div><div class="tile" id="blend"></div><div class="tile" id="layers"></div>
	<div class="tile" id="padding"></div><div class="tile" id="var"></div><div class="tile" id="alias"></div>
	<div class="tile" id="reset"></div><div class="tile" id="math"></div><div class="tile" id="clipped"></div>
	<div class="tile" id="flipped"></div><div class="tile" id="zero"></div>
	<div class="tile" id="fractional"></div>"""
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	_check(layout.parse(source) == OK and layout.layout(Vector2i(480, 300)) == OK, "Lay out spaced and rounded backgrounds")
	_check(_has_diagnostic(layout, "background-repeat", "repeat-x space", "style_element") and
		_has_diagnostic(layout, "background-repeat-x", "repeat-y", "style_element"), "Mixed directional keywords are rejected")
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(480, 300)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(480.0, 300.0)
	label.text = source
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var output: Image = viewport.get_texture().get_image()
	var clear: Color = Color("#303030")
	var samples: Array[Array] = [
		["spaced", Vector2(0.0, 12.0), Color.RED, "Space anchors the first tile and ignores horizontal position"],
		["spaced", Vector2(8.0, 12.0), clear, "Space shows a horizontal gap"],
		["spaced", Vector2(43.0, 18.0), Color.RED, "The last tile touches the positioning area's far edge"],
		["spaced", Vector2(9.0, 19.0), clear, "The vertical no-repeat axis honors its position"],
		["few", Vector2(4.0, 3.0), clear, "A single spaced image honors the requested position"],
		["few", Vector2(5.0, 3.0), Color.RED, "The fallback tile touches the right edge"],
		["oversized", Vector2(0.0, 2.0), Color.RED, "An oversized tile can be clipped on the fallback axis"],
		["oversized", Vector2(12.0, 2.0), Color.GREEN, "The oversized image is aligned at the requested edge"],
		["both_space", Vector2(43.0, 43.0), Color.RED, "Space fills both dimensions to the last edges"],
		["both_space", Vector2(7.0, 10.0), clear, "Space creates an independent vertical gap"],
		["round_x", Vector2(43.0, 6.0), Color.RED, "Round fits three tiles across the full width"],
		["round_x", Vector2(43.0, 7.0), clear, "An automatic height follows the rounded tile width"],
		["round_y", Vector2(27.0, 42.0), Color.RED, "Round fits a whole number of tiles vertically"],
		["round_y", Vector2(29.0, 42.0), clear, "An automatic width follows the rounded tile height"],
		["round_both", Vector2(14.0, 13.0), Color.YELLOW, "Round keeps fractional horizontal tile edges"],
		["round_both", Vector2(15.0, 14.0), Color.RED, "The second tile starts in both axes"],
		["round_position", Vector2(0.0, 1.0), Color.GREEN, "Round uses the scaled size to resolve position"],
		["round_position", Vector2(10.0, 1.0), Color.RED, "The rounded repeat begins at the resolved position"],
		["gradient", Vector2(7.0, 1.0), Color.BLUE, "Gradient sizing uses the unspaced tile width"],
		["gradient", Vector2(8.0, 1.0), clear, "A gradient tile leaves the same gap as a texture"],
		["gradient", Vector2(9.0, 1.0), Color.RED, "A gradient restarts after its gap"],
		["fallback", Vector2(8.0, 1.0), clear, "A solid fallback leaves the same gap"],
		["fallback", Vector2(9.0, 1.0), Color.RED, "A solid fallback fills a spaced tile"],
		["blend", Vector2(8.0, 1.0), clear, "A cross-fade leaves each gap transparent"],
		["blend", Vector2(9.0, 1.0), Color("#ff8080"), "A cross-fade repeats the blended tile"],
		["layers", Vector2(9.0, 3.0), Color.RED, "First layer uses the first horizontal repetition value"],
		["layers", Vector2(11.0, 13.0), clear, "Second layer uses the second horizontal repetition value"],
		["layers", Vector2(43.0, 25.0), Color.BLUE, "Third layer cycles the horizontal list independently"],
		["padding", Vector2(0.0, 3.0), Color.RED, "Space extends its pattern past a narrower positioning area"],
		["padding", Vector2(7.0, 3.0), clear, "The extended pattern preserves its gap"],
		["padding", Vector2(8.0, 3.0), Color.RED, "Space uses the content edge as its origin"],
		["var", Vector2(8.0, 20.0), clear, "The winning horizontal component still comes from var()"],
		["var", Vector2(9.0, 20.0), Color.RED, "The logical block longhand overrides only the vertical component"],
		["alias", Vector2(3.0, 16.0), Color.RED, "Repeat-block tiles vertically in horizontal writing"],
		["alias", Vector2(9.0, 16.0), clear, "Repeat-block places only one tile horizontally"],
		["reset", Vector2(19.0, 16.0), Color.RED, "Background shorthand restores repeat on both axes"],
		["math", Vector2(33.0, 7.0), clear, "Math positions keep the gap before a tile"],
		["math", Vector2(34.0, 7.0), Color.RED, "Math positions and sizes resolve against their correct bases"],
		["clipped", Vector2(0.0, 0.0), clear, "Rounded clipping masks the gapped image shader"],
		["clipped", Vector2(3.0, 8.0), Color.RED, "Spaced tiles paint inside the rounded clip"],
		["flipped", Vector2(0.0, 1.0), Color.GREEN, "Direction flips each tile's pixels"],
		["flipped", Vector2(8.0, 1.0), clear, "A flipped image still leaves its gap at the same position"],
		["flipped", Vector2(9.0, 1.0), Color.GREEN, "Each spaced directional tile starts on its flipped side"],
		["zero", Vector2(0.0, 1.0), clear, "A zero-size tile paints nothing"],
		["fractional", Vector2(20.0, 1.0), Color.RED, "Space keeps three mathematically exact fractional tiles"],
	]
	for sample: Array in samples:
		var id: String = sample[0]
		var offset: Vector2 = sample[1]
		var expected: Color = sample[2]
		var description: String = sample[3]
		_check_pixel(output, _label_rect(label, id).position + offset, expected, description)
	viewport.queue_free()


func _check_image_positions() -> void:
	var source: String = """<!doctype html><style>
		body { margin: 0; background: #303030; display: grid; grid-template-columns: repeat(5, 96px);
			grid-auto-rows: 66px; align-items: start; justify-items: start; }
		.tile { width: 56px; height: 48px; background-color: #303030;
			background-image: image(red); background-size: 10px 8px;
			background-repeat: no-repeat; image-rendering: crisp-edges; }
		#four { background-position: right 12px bottom 8px; }
		#reversed { background: image(lime) bottom 25% right calc(10% + 1.4px) / 10px 8px no-repeat #303030; }
		#three { background-position: bottom 5px left; }
		#center_right { background-position: center right 8px; }
		#negative_free_space { background-image: url(orientation-1.png); background-size: 64px 16px;
			background-position: right 25% center; }
		#negative_offset { background-position: left -3px bottom -2px; }
		#legacy_pair { background-position: right 7px; background-position: top 7px; }
		#layers { background-image: image(red), image(lime), image(blue); background-size: 8px 6px;
			background-position-x: right 4px, left 12px; background-position-y: top 4px, center, bottom 3px; }
		#var { --position: right 9px bottom 4px; background-position: var(--position);
			background-position-x: left 11px !important; background-position-y: right 1px; }
		#invalid { background-position: right 6px top 5px;
			background-position: left 2px right 3px; background-position-x: 6px right; }
		#logical_pair { direction: rtl; background-position: x-start y-end; }
		#logical_axes { direction: rtl; background-position-inline: start 3px; background-position-block: end 6px; }
		#logical_offset { direction: rtl; background-position-inline: 6px; background-position-block: 7px; }
		#logical_four { direction: rtl; background-position: block-end 4px inline-end 8px; }
		#generic_four { direction: rtl; background-position: end 4px start 2px; }
		#logical_initial { direction: rtl; background-position: left top; background-position-inline: initial; }
		#parent { direction: rtl; background-position: inline-start 4px block-end 8px; }
		#inherited { direction: ltr; background-position: inherit; }
		#round { width: 60px; background: url(orientation-1.png) right 4px bottom 3px / 24px 10px round no-repeat; }
		#space { width: 16px; background-position: right 2px bottom 4px; background-repeat: space no-repeat; }
		#origin { padding: 5px; border: 2px solid transparent; background-origin: content-box;
			background-position: right 12px bottom 8px; }
		#object { width: 56px; height: 48px; background: #303030; image-resolution: 2x;
			image-rendering: crisp-edges; direction: rtl; object-fit: none; object-position: inline-end 6px block-end 7px; }
		#reset { background-position-x: right 8px; background-position-block: end;
			background: image(red) no-repeat; background-size: 10px 8px; }
		#incompatible { background-position: left 9px top 6px; background-position: left block-end; }
		#logical_var { direction: rtl; --pos: start 3px; background-position-inline: var(--pos);
			background-position-block: var(--missing, end 5px); }
	</style>
	<div class="tile" id="four"></div><div class="tile" id="reversed"></div><div class="tile" id="three"></div>
	<div class="tile" id="center_right"></div><div class="tile" id="negative_free_space"></div>
	<div class="tile" id="negative_offset"></div><div class="tile" id="legacy_pair"></div><div class="tile" id="layers"></div>
	<div class="tile" id="var"></div><div class="tile" id="invalid"></div>
	<div class="tile" id="logical_pair"></div><div class="tile" id="logical_axes"></div><div class="tile" id="logical_offset"></div>
	<div class="tile" id="logical_four"></div><div class="tile" id="generic_four"></div>
	<div id="parent"><div class="tile" id="inherited"></div></div>
	<div class="tile" id="logical_initial"></div><div class="tile" id="round"></div><div class="tile" id="space"></div>
	<div class="tile" id="origin"></div><img id="object" src="orientation-1.png">
	<div class="tile" id="reset"></div><div class="tile" id="incompatible"></div><div class="tile" id="logical_var"></div>"""
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	_check(layout.parse(source) == OK and layout.layout(Vector2i(480, 330)) == OK, "Lay out edge offsets and position longhands")
	_check(_has_diagnostic(layout, "background-position", "top 7px", "style_element") and
		_has_diagnostic(layout, "background-position", "left 2px right 3px", "style_element") and
		_has_diagnostic(layout, "background-position", "left block-end", "style_element") and
		_has_diagnostic(layout, "background-position-y", "right 1px", "style_element") and
		_has_diagnostic(layout, "background-position-x", "6px right", "style_element"),
		"Conflicting axes, mixed keyword families, and reversed offsets produce diagnostics")
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(480, 330)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(480.0, 330.0)
	label.text = source
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var output: Image = viewport.get_texture().get_image()
	var clear: Color = Color("#303030")
	var samples: Array[Array] = [
		["four", Vector2(34.0, 32.0), Color.RED, "Offsets are measured inward from right and bottom"],
		["four", Vector2(33.0, 32.0), clear, "The tile begins exactly at the right offset"],
		["reversed", Vector2(40.0, 30.0), Color.LIME, "Reversed pairs work in the background shorthand using math"],
		["reversed", Vector2(39.0, 30.0), clear, "Percentage offset uses the remaining horizontal space"],
		["three", Vector2(0.0, 35.0), Color.RED, "An omitted offset is zero on its edge"],
		["center_right", Vector2(38.0, 20.0), Color.RED, "Center can pair with an edge and offset"],
		["negative_free_space", Vector2(25.0, 17.0), Color.RED, "Percentages work with negative free space"],
		["negative_free_space", Vector2(26.0, 17.0), Color.GREEN, "The resulting tile aligns at the negative basis"],
		["negative_offset", Vector2(0.0, 41.0), clear, "A negative bottom offset moves the tile outward"],
		["negative_offset", Vector2(0.0, 42.0), Color.RED, "The left edge can be moved outward independently"],
		["legacy_pair", Vector2(46.0, 7.0), Color.RED, "A two-value position still assigns its length to the vertical axis"],
		["layers", Vector2(44.0, 4.0), Color.RED, "First axis list entry locates the first layer"],
		["layers", Vector2(12.0, 21.0), Color.LIME, "Each axis computes its own list position"],
		["layers", Vector2(44.0, 39.0), Color.BLUE, "A short horizontal list cycles independently"],
		["var", Vector2(11.0, 36.0), Color.RED, "Substituted position preserves the winning physical longhand"],
		["invalid", Vector2(40.0, 5.0), Color.RED, "Invalid positions preserve the earlier valid declaration"],
		["logical_pair", Vector2(46.0, 40.0), Color.RED, "Flow-relative axis keywords read RTL and horizontal writing"],
		["logical_axes", Vector2(43.0, 34.0), Color.RED, "Logical longhands resolve offsets from their requested edges"],
		["logical_offset", Vector2(40.0, 7.0), Color.RED, "A logical length alone starts on the logical start side"],
		["logical_four", Vector2(8.0, 36.0), Color.RED, "Logical edge pairs are accepted in either axis order"],
		["generic_four", Vector2(44.0, 36.0), Color.RED, "Unqualified edge pairs are in block then inline order"],
		["inherited", Vector2(42.0, 32.0), Color.RED, "Inheritance uses the parent's computed physical edges"],
		["logical_initial", Vector2(46.0, 0.0), Color.RED, "Logical initial comes from the inline start side"],
		["round", Vector2(15.0, 36.0), Color.GREEN, "Right offset is based on the rounded tile's width"],
		["round", Vector2(16.0, 36.0), Color.RED, "Rounded tiles repeat from the same resolved origin"],
		["space", Vector2(3.0, 37.0), clear, "One spaced tile honors edge offsets"],
		["space", Vector2(4.0, 37.0), Color.RED, "The single tile begins at the right offset"],
		["origin", Vector2(41.0, 39.0), Color.RED, "Edge offsets are relative to the selected background origin"],
		["object", Vector2(5.0, 32.0), clear, "Object positioning uses the same edge geometry"],
		["object", Vector2(6.0, 32.0), Color.RED, "Object direction resolves logical edges with the fitted size"],
		["reset", Vector2(0.0, 0.0), Color.RED, "Background shorthand resets both position axes"],
		["incompatible", Vector2(9.0, 6.0), Color.RED, "An incompatible logical pair leaves the earlier declaration active"],
		["logical_var", Vector2(43.0, 35.0), Color.RED, "Logical single-axis shorthands accept var() and its fallback"],
	]
	for sample: Array in samples:
		var id: String = sample[0]
		var offset: Vector2 = sample[1]
		var expected: Color = sample[2]
		var description: String = sample[3]
		_check_pixel(output, _label_rect(label, id).position + offset, expected, description)
	viewport.queue_free()


func _check_background_attachment() -> void:
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(300, 290)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(260.0, 104.0)
	label.text = """<!doctype html><style>
		body { margin: 0; background: #303030; }
		.panel { position: absolute; top: 20px; width: 110px; height: 116px; }
		#fixed { left: 16px; padding: 7px; border-radius: 8px; image-rendering: crisp-edges;
			background: url(density-live.png) 44px 48px / 20px 18px fixed no-repeat content-box border-box; }
		#scroll { left: 144px; background: image(blue) 10px 28px / 20px 18px scroll no-repeat; }
		#fixed_gradient { position: absolute; top: 15px; left: 120px; width: 23px; height: 120px;
			background: linear-gradient(lime, lime) 123px 50px / 8px 12px fixed no-repeat; }
		#spacer { width: 340px; height: 230px; }
	</style><div id="spacer"></div><div class="panel" id="fixed"></div>
	<div class="panel" id="scroll"></div><div id="fixed_gradient"></div>"""
	viewport.add_child(label)
	var nested: HTMLLabel = HTMLLabel.new()
	nested.position = Vector2(0.0, 124.0)
	nested.size = Vector2(290.0, 150.0)
	nested.text = """<!doctype html><style>
		body { margin: 0; background: #303030; }
		.inner { position: absolute; top: 10px; width: 72px; height: 46px; overflow: auto; }
		.content { width: 132px; height: 142px; }
		#local { left: 10px; background: image(red) 10px 55px / 20% 8px local no-repeat; }
		#scroll_inner { left: 100px; background: image(blue) 10px 19px / 20% 8px scroll no-repeat; }
		#inner_fixed { left: 190px; }
		#inner_fixed_child { height: 142px;
			background: cross-fade(lime, lime) 205px 25px / 12px 10px fixed no-repeat; }
		#hidden { left: 10px; top: 70px; overflow: hidden;
			background: image(lime) 10px 12px / 20% 8px local no-repeat; }
		#transform { position: absolute; left: 100px; top: 70px; transform: translate(8px, 5px); }
		#transform_child { width: 72px; height: 46px; background: image(red) 9px 10px / 12px 8px fixed no-repeat; }
	</style>
	<div id="local" class="inner"><div id="local_content" class="content"></div></div>
	<div id="scroll_inner" class="inner"><div id="scroll_content" class="content"></div></div>
	<div id="inner_fixed" class="inner"><div id="inner_fixed_child"></div></div>
	<div id="hidden" class="inner"><div id="hidden_content" class="content"></div></div>
	<div id="transform"><div id="transform_child"></div></div>"""
	viewport.add_child(nested)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var output: Image = viewport.get_texture().get_image()
	var clear: Color = Color("#303030")
	var fixed_point: Vector2 = Vector2(45.0, 49.0)
	var gradient_point: Vector2 = Vector2(124.0, 51.0)
	_check_pixel(output, fixed_point, Color.RED, "Fixed texture ignores the element's content-box origin")
	_check_pixel(output, gradient_point, Color.LIME, "Fixed gradient starts from the viewport origin")
	_check_pixel(output, Vector2(155.0, 49.0), Color.BLUE, "Scroll attachment starts from the element's box")
	_check_pixel(output, nested.position + Vector2(22.0, 30.0), clear, "Local tile starts below the inner viewport")
	_check_pixel(output, nested.position + Vector2(112.0, 30.0), Color.BLUE, "Scroll background is initially visible inside its box")
	_check_pixel(output, nested.position + Vector2(206.0, 27.0), Color.LIME, "Fixed cross-fade is initially visible through the inner clip")
	_check_pixel(output, nested.position + Vector2(44.0, 83.0), Color.LIME, "Hidden overflow still sets the local positioning area's size")
	var transformed_box: Rect2 = _label_rect(nested, "transform_child")
	_check_pixel(output, nested.position + transformed_box.position + Vector2(10.0, 11.0), Color.RED,
		"A transformed ancestor makes a descendant's fixed attachment use the element box")

	var inner_content: HTMLElement = nested.get_element_by_id("local_content")
	var inner_y: float = inner_content.get_bounding_rect().position.y
	var original_extent: Vector2i = nested.get_content_size()
	var hidden_before: Rect2 = _label_rect(nested, "hidden_content")
	_push_html_wheel(viewport, nested.position + Vector2(24.0, 34.0), MOUSE_BUTTON_WHEEL_DOWN)
	_push_html_wheel(viewport, nested.position + Vector2(114.0, 34.0), MOUSE_BUTTON_WHEEL_DOWN)
	_push_html_wheel(viewport, nested.position + Vector2(206.0, 34.0), MOUSE_BUTTON_WHEEL_DOWN)
	_push_html_wheel(viewport, nested.position + Vector2(24.0, 86.0), MOUSE_BUTTON_WHEEL_DOWN)
	label.set_scroll_offset(Vector2i(12, 30))
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	output = viewport.get_texture().get_image()
	_check(label.get_scroll_offset() == Vector2i(12, 30), "The label scrolls on both axes")
	_check_pixel(output, fixed_point, Color.RED, "Fixed image stays pinned during native label scrolling")
	_check_pixel(output, gradient_point, Color.LIME, "Fixed gradient stays pinned during native label scrolling")
	_check_pixel(output, Vector2(143.0, 19.0), Color.BLUE, "Scroll background moves with the outer box")
	_check_pixel(output, Vector2(45.0, 19.0), clear, "The no-repeat fixed image does not repeat outside its viewport tile")
	_check(is_equal_approx(inner_content.get_bounding_rect().position.y, inner_y - 36.0),
		"Wheel input moves the content of the inner scroll box")
	_check(_label_rect(nested, "hidden_content").is_equal_approx(hidden_before), "Hidden overflow does not consume wheel input")
	_check(nested.get_content_size() == original_extent and nested.get_scroll_offset() == Vector2i.ZERO,
		"Inner scrolling preserves the document extent and outer offset")
	_check_pixel(output, nested.position + Vector2(44.0, 30.0), Color.RED, "The local tile scrolls into view and sizes against the scrollable width")
	_check_pixel(output, nested.position + Vector2(47.0, 30.0), clear, "The scrollable width still bounds the local tile")
	_check_pixel(output, nested.position + Vector2(112.0, 30.0), Color.BLUE, "The scroll attachment stays at the inner border")
	_check_pixel(output, nested.position + Vector2(124.0, 30.0), clear, "The scroll attachment sizes against the visible box")
	_check_pixel(output, nested.position + Vector2(206.0, 27.0), Color.LIME, "The fixed cross-fade also stays pinned in a nested scroller")

	var local: HTMLElement = nested.get_element_by_id("local")
	_check(local.set_attribute("data-changed", "yes") == OK, "Restyle the already scrolled element")
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	_check(is_equal_approx(inner_content.get_bounding_rect().position.y, inner_y - 36.0),
		"Stylesheet changes restore the element's scroll offset")
	_check_pixel(viewport.get_texture().get_image(), nested.position + Vector2(44.0, 30.0), Color.RED,
		"Restyling keeps the local attachment at the same scrolled position")
	var content_left: float = inner_content.get_bounding_rect().position.x
	_check(local.set_attribute("style", "background-position-x: right 10px") == OK, "Move the local tile near the scrollable right edge")
	await process_frame
	await process_frame
	_push_html_wheel(viewport, nested.position + Vector2(24.0, 34.0), MOUSE_BUTTON_WHEEL_RIGHT)
	await process_frame
	await RenderingServer.frame_post_draw
	_check(is_equal_approx(inner_content.get_bounding_rect().position.x, content_left - 36.0),
		"Horizontal wheel input moves the inner content")
	_check_pixel(viewport.get_texture().get_image(), nested.position + Vector2(71.0, 30.0), Color.RED,
		"Horizontal scrolling brings the right-aligned local tile into view")
	viewport.queue_free()


func _check_embedded_background_attachment() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	var source: String = """<!doctype html><style>
		body { margin: 0; background: #303030; }
		div { width: 200px; height: 130px; padding: 10px;
			background: image(lime) right 8px bottom 6px / 8px 8px fixed no-repeat content-box border-box; }
	</style><div></div>"""
	_check(layout.parse(source) == OK and layout.layout(Vector2i(164, 100)) == OK, "Lay out a standalone fixed background")
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(240, 200)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var artwork: Node2D = Node2D.new()
	viewport.add_child(artwork)
	artwork.draw.connect(func() -> void:
		layout.draw(artwork, Vector2(11.0, 12.0), Rect2(11.0, 12.0, 164.0, 100.0))
	)

	var label: MarkdownLabel = MarkdownLabel.new()
	label.parse_html = true
	label.position = Vector2(0.0, 122.0)
	label.size = Vector2(170.0, 78.0)
	var test_texture: ImageTexture = ImageTexture.create_from_image(Image.create_empty(1, 1, false, Image.FORMAT_RGBA8))
	for icon: StringName in [&"task_checked", &"task_unchecked", &"file_broken",]:
		label.add_theme_icon_override(icon, test_texture)
	label.text = """<div style="width: 70px; height: 34px; overflow: auto;
		background: #224466 image(red) 5px 42px / 14px 8px local no-repeat;">
		<div style="height: 110px;"></div>
	</div>"""
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var output: Image = viewport.get_texture().get_image()
	_check_pixel(output, Vector2(160.0, 99.0), Color.LIME,
		"Standalone fixed sizes and offsets use the layout viewport and the drawing offset")
	_check_pixel(output, Vector2(158.0, 99.0), Color("#303030"), "Fixed ignores the positioning box even in standalone drawing")
	var top_left: Vector2i = Vector2i(-1, -1)
	for y: int in range(122, 198):
		for x: int in range(160):
			if output.get_pixel(x, y).is_equal_approx(Color("#224466")):
				top_left = Vector2i(x, y)
				break
		if top_left.x >= 0:
			break
	_check(top_left.x >= 0, "Markdown shows the HTML block's scroller")
	if top_left.x >= 0:
		_push_html_wheel(viewport, Vector2(top_left + Vector2i(18, 16)), MOUSE_BUTTON_WHEEL_DOWN)
		await process_frame
		await RenderingServer.frame_post_draw
		_check_pixel(viewport.get_texture().get_image(), Vector2(top_left + Vector2i(6, 7)), Color.RED,
			"The Markdown HTML block scrolls its local background")
	viewport.queue_free()


func _push_html_wheel(viewport: SubViewport, position: Vector2, index: MouseButton) -> void:
	var motion: InputEventMouseMotion = InputEventMouseMotion.new()
	motion.position = position
	motion.global_position = position
	viewport.push_input(motion, true)
	var event: InputEventMouseButton = InputEventMouseButton.new()
	event.button_index = index
	event.pressed = true
	event.position = position
	event.global_position = position
	event.factor = 1.0
	viewport.push_input(event, true)


func _write_image_dpi_fixtures() -> bool:
	var tiff: PackedByteArray = []
	tiff.resize(66)
	tiff[0] = 0x49
	tiff[1] = 0x49
	tiff.encode_u16(2, 42)
	tiff.encode_u32(4, 8)
	tiff.encode_u16(8, 3)
	for index: int in range(2):
		var entry: int = 10 + index * 12
		var rational: int = 50 + index * 8
		tiff.encode_u16(entry, 0x011a + index)
		tiff.encode_u16(entry + 2, 5)
		tiff.encode_u32(entry + 4, 1)
		tiff.encode_u32(entry + 8, rational)
		tiff.encode_u32(rational, 384)
		tiff.encode_u32(rational + 4, 1)
	tiff.encode_u16(34, 0x0128)
	tiff.encode_u16(36, 3)
	tiff.encode_u32(38, 1)
	tiff.encode_u16(42, 2)
	var jpeg: PackedByteArray = [0xff, 0xd8, 0xff, 0xe1, 0, 74, 69, 120, 105, 102, 0, 0,]
	jpeg.append_array(tiff)
	jpeg.append_array(PackedByteArray([0xff, 0xd9,]))
	var webp: PackedByteArray = "RIFF".to_ascii_buffer()
	webp.resize(20)
	webp.encode_u32(4, tiff.size() + 12)
	for index: int in range(8):
		webp[8 + index] = "WEBPEXIF".unicode_at(index)
	webp.encode_u32(16, tiff.size())
	webp.append_array(tiff)
	var late_webp: PackedByteArray = webp.slice(0, 12)
	late_webp.encode_u32(4, tiff.size() + 20)
	late_webp.append_array(PackedByteArray([86, 80, 56, 76, 0, 0, 0, 0,]))
	late_webp.append_array(webp.slice(12))
	var jfif: PackedByteArray = [0xff, 0xd8, 0xff, 0xe0, 0, 16, 74, 70, 73, 70, 0, 1, 2, 1,
		1, 32, 1, 32, 0, 0, 0xff, 0xd9,]
	var files: Dictionary[String, PackedByteArray] = {
		"res://density-jfif.jpg": jfif,
		"res://density-exif.jpg": jpeg,
		"res://density-exif.webp": webp,
		"res://density-late.webp": late_webp,
	}
	for path: String in files:
		var file: FileAccess = FileAccess.open(path, FileAccess.WRITE)
		if file == null or not file.store_buffer(files[path]):
			return false
	return true


func _check_generated_content() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var html: String = r"""<!doctype html><html lang="de"><style>
body { margin: 0; counter-reset: chapter 1; }
q::before { content: open-quote; }
q::after { content: close-quote; }
.brackets { quotes: "[" "]" "(" ")"; }
.silent::before { content: no-open-quote; }
.silent::after { content: no-close-quote; }
.no_quotes { quotes: none; }
.auto_quotes { quotes: auto; }
.early::before { content: close-quote "EARLY"; }
.label::before { content: "<\"" attr(data-name) "\" >"; }
.fallback::after { content: var(--text, attr(data-missing raw-string, "[fallback]")); }
.empty::after { content: attr(data-empty, "wrong"); }
.bad::before { content: "VALID"; content: counter(Bad, 123); }
.invalid_attr::after { content: "VALIDATTR"; content: attr(data-name bad); }
.count::before { content: "{" counter(chapter, upper-roman) "}"; }
.nested::before { content: "{" counters(chapter, ",", decimal-leading-zero) "}"; }
.negative::after { counter-reset: Negative -1; content: counter(Negative, decimal-leading-zero); }
.hidden { display: none; counter-set: chapter 99; }
.hidden_pseudo::before { content: none; counter-increment: chapter 100; }
.implicit::before { content: counter(Unassigned) "A"; }
.implicit::after { counter-increment: Unassigned 2; content: counter(Unassigned) "B"; }
.responsive { display: none; counter-increment: chapter 4; }
@media (min-width: 400px) { .responsive { display: block; } }
</style><body>
<div class="early"></div>
<div class="brackets"><q>one<q>two<q>three</q></q></q></div>
<div class="brackets"><q class="silent">silent<q>inside</q></q></div>
<div class="no_quotes"><q>plain<q class="auto_quotes">inside</q></q></div>
<div><q>Deutsch</q></div>
<div id="attribute_name" class="label" data-name="A&#92;B">value</div>
<div class="fallback">missing</div><div class="empty" data-empty="">empty</div>
<div class="bad">literal</div><div class="invalid_attr">invalid</div>
<div class="count" style="counter-increment: chapter 7; counter-set: chapter 4">start</div>
<div class="hidden"><div style="counter-increment: chapter 2"></div></div>
<div class="hidden_pseudo"></div>
<div style="counter-reset: chapter 2"><div class="nested" style="counter-increment: chapter">inner</div></div>
<div class="nested" style="counter-reset: chapter 6">replace</div>
<div class="responsive"></div><div class="count">final</div>
<div class="implicit"></div>
<div class="negative">negative</div>
</body></html>"""
	_check(layout.parse(html) == OK and layout.layout(Vector2i(340, 460)) == OK, "Lay out generated content")
	var rendered: String = layout.get_plain_text()
	for snippet: String in ["EARLY", "[one(two(three))]", "silent(inside)", "plain‚inside‘", "„Deutsch“",
			'<"A\\B" >value', "missing[fallback]", "empty", "VALIDliteral", "invalidVALIDATTR", "{IV}start",
			"{04,03}inner", "{04,06}replace", "{VI}final", "0A2B", "negative-1"]:
		_check(snippet in rendered, "Generated text contains %s" % snippet)
	_check("wrong" not in rendered and "‚EARLY" not in rendered, "Empty attributes and unmatched closing quotes")
	_check(layout.get_element_by_id("attribute_name").get_text_content() == "value", "Generated content stays outside DOM text")
	_check(layout.get_css_diagnostics().size() == 2, "Invalid content functions produce diagnostics: %s" % [layout.get_css_diagnostics()])
	_check(layout.layout(Vector2i(350, 460)) == OK and layout.get_plain_text() == rendered,
		"Reflow keeps quotes and counters reproducible without a style change")
	_check(layout.layout(Vector2i(420, 460)) == OK and "{X}final" in layout.get_plain_text(),
		"Media changes recalculate counters in document order")
	var attribute: HTMLElement = layout.get_element_by_id("attribute_name")
	_check(attribute.set_attribute("data-name", 'C)"D') == OK and layout.layout(Vector2i(420, 460)) == OK and
		'<"C)"D" >value' in layout.get_plain_text(), "Updating an attribute refreshes generated text literally")


func _check_generated_image() -> void:
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(160, 100)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)

	var texture_image: Image = Image.create_empty(16, 12, false, Image.FORMAT_RGBA8)
	texture_image.fill(Color("#389f89"))
	var texture: ImageTexture = ImageTexture.create_from_image(texture_image)
	var document: HTMLDocument = HTMLDocument.new()
	document.text = '<link rel="stylesheet" href="styles/sheet.css"><div id="generated_image"></div>'
	document.set("_textures", { "res://styles/nested/picture.png": texture, })
	document.set("_local_stylesheets", {
		"res://styles/sheet.css": '@import "nested/content.css"; ',
		"res://styles/nested/content.css": '''
			body { margin: 0; }
			#generated_image::before { content: var(--picture); }
			#generated_image { --picture: url("picture.png"); }
		''',
	})
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(140.0, 90.0)
	label.document = document
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	_check(_color_bounds(image, Rect2i(Vector2i.ZERO, viewport.size), Color("#389f89")).size == Vector2i(16, 12),
		"A nested stylesheet resolves generated images from its own directory")
	_check(label.query_selector("img") == null, "A generated image stays outside DOM queries")
	var image_owner: HTMLElement = label.get_element_at(Vector2(5.0, 5.0))
	_check(image_owner != null and image_owner.get_attribute("id") == "generated_image", "Generated image hits return the originating element")
	viewport.queue_free()


func _check_list_markers() -> void:
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(320, 310)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)

	var html: String = """<!doctype html><style>
body { margin: 0; }
ol, ul { margin: 0; padding-left: 72px; }
li { line-height: 24px; }
li::marker { content: '(' counters(list-item, '.', decimal) ') '; color: #a020d0; font-weight: bold; }
li::before { content: '|'; }
#inside { list-style-position: inside; }
#hidden_marker::marker { content: none; }
#default_marker::marker { content: normal; color: #259955; }
#no_default::marker, #typed::marker, #typed_override::marker, #text_marker::marker, #quoted_type::marker { content: normal; }
#typed { list-style-type: revert; }
#typed_override { list-style-type: decimal; }
#text_marker { list-style: inside '>> '; }
#quoted_type { list-style-type: '§ '; }
#inside::marker { content: attr(data-label) ' '; color: #cf281f; display: none; width: 99px; }
#block_child::marker { content: '[B]'; color: #1735cd; }
#ordinary::marker { content: 'WRONG'; }
#step { counter-increment: list-item 2; }
</style>
<ol start="3">
  <li id="outside"><span id="outside_word">First</span><ol><li>Nested</li></ol></li>
  <li id="inside" data-label="Wide marker" value="8"><span id="inside_word">Second</span></li>
  <li id="step">Step</li>
  <li id="hidden_marker">Suppressed</li>
  <li id="default_marker">Default</li>
  <li id="block_child"><div>Block child</div></li>
</ol>
<ol reversed><li>Descending</li><li>Last</li></ol>
<ol type="A" start="4"><li id="typed">Typed</li><li id="typed_override">CSS type</li></ol>
<ul><li id="text_marker">Literal</li><li id="quoted_type">Quoted</li><li id="no_default" style="list-style-type: none">No default</li></ul>
<div id="ordinary">Ordinary</div>"""
	var layout: HTMLLayout = HTMLLayout.new()
	_check(layout.parse(html) == OK and layout.layout(Vector2i(300, 300)) == OK, "Lay out list markers")
	var text: String = layout.get_plain_text()
	for fragment: String in ["(3) |First", "(3.1) |Nested", "Wide marker |Second", "(10) |Step",
			"|Suppressed", "12. |Default", "[B]", "(2) |Descending", "(1) |Last", "D. |Typed", "5. |CSS type",
			">> |Literal", "§ |Quoted", "|No default", "Ordinary"]:
		_check(fragment in text, "Rendered markers contain %s" % fragment)
	_check("WRONG" not in text and "(11)" not in text, "Marker rules only generate visible markers for list items")
	_check(layout.get_element_by_id("inside").get_text_content() == "Second", "Markers stay outside DOM text")
	_check(layout.query_selector_all("ol > li:first-child").size() == 4, "Marker nodes stay outside structural selectors")
	_check(_rect(layout, "inside_word").position.x > _rect(layout, "outside_word").position.x + 55.0,
		"An inside marker occupies its text width in the first line")
	var inside: HTMLElement = layout.get_element_by_id("inside")
	_check(inside.set_attribute("data-label", "New") == OK and layout.layout(Vector2i(290, 300)) == OK and
		"New |Second" in layout.get_plain_text(), "Attribute changes refresh marker content")

	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(320.0, 310.0)
	label.text = html
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	var outside: Rect2 = _label_rect(label, "outside")
	var outside_pixels: Rect2i = _color_bounds(image, Rect2i(0, 0, 120, 24), Color("#a020d0"))
	_check(outside_pixels.has_area() and outside_pixels.end.x < outside.position.x,
		"An outside marker paints to the left of the list item")
	var inside_pixels: Rect2i = _color_bounds(image, Rect2i(Vector2i.ZERO, viewport.size), Color("#cf281f"))
	_check(inside_pixels.has_area() and inside_pixels.position.x >= _label_rect(label, "inside").position.x,
		"An inside marker inherits text styling and paints inside the content box")
	var block_pixels: Rect2i = _color_bounds(image, Rect2i(Vector2i.ZERO, viewport.size), Color("#1735cd"))
	_check(block_pixels.has_area() and block_pixels.end.x < _label_rect(label, "block_child").position.x,
		"An outside marker also paints beside block content")
	viewport.queue_free()


func _check_counter_styles() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var html: String = """<!doctype html><style>
@import "styles/counters.css" layer(imported);
@layer imported, preferred;
@layer preferred {
  @counter-style layered { system: cyclic; symbols: high; }
}
@layer imported {
  @counter-style layered { system: cyclic; symbols: low; }
}
@counter-style digits { system: numeric; symbols: X Y; prefix: '{'; suffix: '} '; negative: '(' ')'; pad: 5 '0'; }
@counter-style limited { system: fixed 2; symbols: A B; range: 2 2; fallback: upper-roman; prefix: '['; suffix: '] '; }
@counter-style inherited { system: extends digits; prefix: '<'; suffix: '> '; }
@counter-style additive { system: additive; additive-symbols: 5 V, 2 X, 0 Z; fallback: decimal; }
@counter-style cycle-one { system: extends cycle-two; suffix: ') '; }
@counter-style cycle-two { system: extends cycle-one; pad: 3 '0'; }
@counter-style fallback-one { system: fixed; symbols: A; fallback: fallback-two; }
@counter-style fallback-two { system: fixed; symbols: B; fallback: fallback-one; }
@counter-style Repeat { symbols: R S; }
@counter-style repeat { system: cyclic; symbols: small; }
@counter-style repeated-rule { system: fixed; symbols: early; prefix: '!'; }
@counter-style repeated-rule { system: cyclic; symbols: late; symbols: 42; }
@counter-style bounded-output { system: symbolic; symbols: 'x'; prefix: '['; suffix: '] '; }
@counter-style max-int { system: fixed 2147483647; symbols: max; }
@counter-style responsive { system: fixed; symbols: narrow; }
@media (min-width: 400px) { @counter-style responsive { system: cyclic; symbols: wide; } }
@supports (list-style-type: symbols(cyclic '*')) { @counter-style conditional { symbols: YES; } }
@counter-style decimal { system: cyclic; symbols: WRONG; }
body { margin: 0; }
ol { margin: 0; padding: 0; list-style-position: inside; }
.digits { list-style-type: var(--counter-style, digits); }
.limited { list-style-type: limited; }
.inherited { list-style: inherited inside; }
.literal::before { counter-reset: sample 7; content: counter(sample, additive) '/' counter(sample, Repeat) '/'
  counter(sample, repeat) '/' counter(sample, cycle-two) '/' counter(sample, fallback-one) '/'
  counter(sample, symbols(alphabetic 'a' 'b')) '/' counter(sample, missing-style) ': '; }
.named::before { counter-reset: sample 1; content: counter(sample, layered) ' ' counter(sample, repeated-rule) ' '
  counter(sample, responsive) ' ' counter(sample, conditional) ' ' counter(sample, imported-counter) ' '; }
.bad::before { content: 'valid '; content: counter(sample, symbols(numeric 'one')); }
.local::before { counter-reset: sample 32; content: counter(sample, CJK-DECIMAL) '/' counter(sample, arabic-indic) '/'
  counter(sample, cjk-heavenly-stem) ': '; }
.large::before { counter-reset: sample 2000000000; content: counter(sample, bounded-output) ': '; }
.precision::before { counter-reset: sample 2147483647; content: counter(sample, max-int) ': '; }
</style>
<ol class="digits" start="-3"><li>minus</li></ol>
<ol class="inherited" start="3"><li>extended</li></ol>
<ol class="limited" start="2"><li>fixed</li><li>fallback</li></ol>
<ol style="list-style-type: symbols(cyclic 'red' 'blue')" start="0"><li>anonymous</li></ol>
<ol style="list-style-type: cycle-one"><li>cycle</li></ol>
<div class="literal">values</div><div class="named">names</div><div class="bad">invalid</div>
<div class="local">local</div><div class="large">large</div><div class="precision">precise</div>"""
	_check(layout.parse(html) == OK and layout.layout(Vector2i(360, 320)) == OK, "Lay out custom counter styles")
	var rendered: String = layout.get_plain_text()
	for fragment: String in ["{(0YY)} minus", "<000YY> extended", "[A] fixed", "[III] fallback", "blue anonymous",
			"1) cycle", "VX/RRRR/small/007/7/aaa/7: values", "high late narrow YES IMPORT names", "valid invalid",
			"三二/٣٢/三二: local", "2000000000: large", "max: precise"]:
		_check(fragment in rendered, "Custom counter output contains %s" % fragment)
	_check(layout.get_css_diagnostics().size() == 1, "Invalid symbols() keeps the previous content declaration")
	_check(layout.layout(Vector2i(430, 320)) == OK and "high late wide YES IMPORT names" in layout.get_plain_text(),
		"Media changes update counter style definitions")
	var separate: HTMLLayout = HTMLLayout.new()
	_check(separate.parse("<ol style='list-style: inside digits'><li>separate</li></ol>") == OK and
		separate.layout(Vector2i(360, 320)) == OK and "1. separate" in separate.get_plain_text(),
		"Counter style definitions stay in their own document")


func _check_font_faces() -> void:
	var document: HTMLDocument = load("{FONT_FACE_UID}")
	_check(document != null, "The HTML importer creates a document with font assets")
	if document == null:
		return
	var assets: Dictionary = document.get("_local_fonts")
	_check(assets.size() == 6 and assets.values().all(func(value: Variant) -> bool: return value is FontFile),
		"HTML imports font resources from every media branch in nested stylesheets")
	if assets.size() != 6:
		return
	var regular: FontFile = assets["res://fonts/times.ttf"]
	var narrow: FontFile = assets["res://fonts/cour.ttf"]
	var bold: FontFile = assets["res://fonts/arialbd.ttf"]
	var italic: FontFile = assets["res://fonts/georgiai.ttf"]
	var layout: HTMLLayout = document.create_layout()
	_check(layout.layout(Vector2i(320, 240)) == OK, "Lay out imported font faces")
	var element: HTMLElement = layout.get_element_by_id("normal")
	_check(is_equal_approx(element.get_bounding_rect().size.x, 7.0 * narrow.get_char_size(48, 40).x),
		"Case-insensitive family and ordered URL fallback select the mobile face")
	_check(is_equal_approx(_rect(layout, "bold").size.x, 7.0 * bold.get_char_size(48, 40).x),
		"Font weight selects the authored bold face")
	_check(is_equal_approx(_rect(layout, "italic").size.x, 7.0 * italic.get_char_size(48, 40).x),
		"Font style selects the authored italic face")
	_check(layout.layout(Vector2i(480, 240)) == OK and element.is_valid(),
		"Reflow across an authored font media breakpoint")
	_check(is_equal_approx(element.get_bounding_rect().size.x, 7.0 * regular.get_char_size(48, 40).x),
		"Changing the active faces invalidates cached font metrics")
	_check(layout.layout(Vector2i(320, 240)) == OK and
		is_equal_approx(element.get_bounding_rect().size.x, 7.0 * narrow.get_char_size(48, 40).x),
		"Reactivating the narrow face restores its metrics")
	_check_font_ranges(assets, regular, narrow, bold, italic)
	_check_local_font_faces(assets, regular, narrow, bold, italic)
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(320, 320)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(320.0, 320.0)
	label.document = document
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	var light: float = _font_ink(image, _label_rect(label, "light"))
	var heavy: float = _font_ink(image, _label_rect(label, "heavy"))
	var above: float = _font_ink(image, _label_rect(label, "above"))
	_check(light > 0.0 and heavy > light * 1.15,
		"The variable font paints thicker glyphs at the higher weight (%s vs %s)" % [heavy, light])
	_check(absf(heavy - above) < 1.0, "The variable weight is clamped to the descriptor range")
	viewport.queue_free()


func _check_local_font_faces(assets: Dictionary, regular: FontFile, narrow: FontFile, bold: FontFile,
		italic: FontFile) -> void:
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_local_fonts", assets)
	document.text = """<!doctype html><style>
		@font-face { font-family: FullName; src: local("Arial Bold"), url(fonts/cour.ttf); }
		@font-face { font-family: PostScript; src: local(Georgia-Italic); font-style: italic; }
		@font-face { font-family: Missing; src: local("Arial Regular"), local("Uninstalled DocRender face"), url(fonts/cour.ttf); }
		@font-face { font-family: WrongStyle; src: local("Georgia"), url(fonts/cour.ttf); font-style: italic; }
		@font-face { font-family: MathFace; src: local(CambriaMath), url(fonts/cour.ttf); }
		@font-face { font-family: CambriaFace; src: local(Cambria), url(fonts/cour.ttf); }
		@font-face { font-family: ProjectMath; src: url(fonts/cambria.ttc#Cambria%4Dath) format(collection); }
		@font-face { font-family: ProjectDefault; src: url(fonts/cambria.ttc) format(collection); }
		@font-face { font-family: InvalidFragment; src: url(fonts/cambria.ttc#Cambria%20Math),
			url(fonts/cambria.ttc#cambriamath), url(fonts/cambria.ttc#missing), url(fonts/cour.ttf); }
		body { margin: 0; font-size: 40px; }
		div { width: 7ch; height: 40px; }
		#full_name { font-family: FullName; }
		#postscript { font-family: PostScript; font-style: italic; }
		#missing { font-family: Missing; }
		#math { font-family: MathFace; }
		#cambria { font-family: CambriaFace; }
		#project_math { font-family: ProjectMath; }
		#project_default { font-family: ProjectDefault; }
		#invalid_fragment { font-family: InvalidFragment; }
		#family_only { font-family: WrongStyle; font-style: italic; }
		#literal { font-family: "Times New Roman"; }
		@font-face { font-family: "Times New Roman"; src: local(inherit), local(serif), local(""); }
	</style>
	<div id="full_name"></div><div id="postscript"></div><div id="missing"></div>
	<div id="math"></div><div id="cambria"></div><div id="family_only"></div><div id="literal"></div>
	<div id="project_math"></div><div id="project_default"></div><div id="invalid_fragment"></div>"""
	var layout: HTMLLayout = document.create_layout()
	_check(layout.layout(Vector2i(420, 440)) == OK, "Lay out installed and project font-face sources")
	var cambria_math: SystemFont = SystemFont.new()
	cambria_math.font_names = PackedStringArray(["Cambria Math"])
	var cambria: SystemFont = SystemFont.new()
	cambria.font_names = PackedStringArray(["Cambria"])
	var georgia: SystemFont = SystemFont.new()
	georgia.font_names = PackedStringArray(["Georgia"])
	var collection: FontFile = assets["res://fonts/cambria.ttc"]
	var project_math: FontFile = FontFile.new()
	project_math.data = collection.data
	project_math.set_face_index(0, 1)
	var server: TextServer = TextServerManager.get_primary_interface()
	var probe: RID = server.create_font()
	server.font_set_data(probe, collection.data)
	server.font_set_face_index(probe, 1)
	var names: Dictionary = server.font_get_ot_name_strings(probe).get("en", {})
	server.free_rid(probe)
	_check(names.get("postscript_name") == "CambriaMath", "Reference the second face in the project collection")
	var expected: Dictionary[String, float] = {
		"full_name": 7.0 * bold.get_char_size(48, 40).x,
		"postscript": 7.0 * italic.get_char_size(48, 40).x,
		"missing": 7.0 * narrow.get_char_size(48, 40).x,
		"math": 7.0 * cambria_math.get_char_size(48, 40).x,
		"cambria": 7.0 * cambria.get_char_size(48, 40).x,
		"project_math": 7.0 * project_math.get_char_size(48, 40).x,
		"project_default": 7.0 * cambria.get_char_size(48, 40).x,
		"invalid_fragment": 7.0 * narrow.get_char_size(48, 40).x,
		"family_only": 7.0 * georgia.get_char_size(48, 40).x,
		"literal": 7.0 * regular.get_char_size(48, 40).x,
	}
	for id: String in expected:
		var actual: float = _rect(layout, id).size.x
		_check(is_equal_approx(actual, expected[id]), "Installed font %s width %s vs %s" % [id, actual, expected[id]])


func _check_font_ranges(assets: Dictionary, regular: FontFile, narrow: FontFile, bold: FontFile,
		italic: FontFile) -> void:
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_local_fonts", assets)
	document.text = """<!doctype html><style>
		@layer base, overrides;
		@layer base {
			@font-face { font-family: Composite; src: url(fonts/times.ttf); unicode-range: U+0-ff; }
		}
		@layer overrides {
			@font-face { font-family: Composite; src: url(fonts/arialbd.ttf); unicode-range: U+4?, U+00E9; }
			@font-face { font-family: Composite; src: url(fonts/cour.ttf); unicode-range: u+49, U+30-39, U+10FFFF;
				unicode-range: U+10f-100; unicode-range: U+110000; unicode-range: U+??????;
				unicode-range: U+4?0; unicode-range: U+4 - 9; }
			@font-face { font-family: Composite; src: url(fonts/georgiai.ttf); unicode-range: U+5A; font-weight: bold; }
		}
		@layer base {
			@font-face { font-family: Composite; src: url(fonts/georgiai.ttf); unicode-range: U+49; }
		}
		@font-face { font-family: Secondary; src: url(fonts/georgiai.ttf); }
		@font-face { font-family: SpaceProbe; src: url(fonts/times.ttf); unicode-range: U+0-ff; }
		@font-face { font-family: SpaceProbe; src: url(fonts/cour.ttf); unicode-range: U+4?; }
		body { margin: 0; font: 40px Composite, Secondary, sans-serif; }
		span { display: inline-block; margin-right: 5px; }
		#metric { width: 7ch; }
		#ranged_height { font-family: SpaceProbe; width: 1ex; height: 1ex; }
		#reference_height { font-family: Composite; width: 1ex; height: 1ex; }
	</style>
	<div><span id="overlap">I</span><span id="wildcard">M</span><span id="base">Z</span>
		<span id="nested">IMZ</span><span id="fallback">Ω</span><span id="accented">é</span></div>
	<div><span id="metric"></span><span id="ranged_height"></span><span id="reference_height"></span></div>"""
	var layout: HTMLLayout = document.create_layout()
	_check(layout.layout(Vector2i(400, 180)) == OK, "Lay out the composite font")
	var expected: Dictionary[String, float] = {
		"overlap": narrow.get_char_size(73, 40).x,
		"wildcard": bold.get_char_size(77, 40).x,
		"base": regular.get_char_size(90, 40).x,
		"fallback": italic.get_char_size(937, 40).x,
		"accented": bold.get_char_size(233, 40).x,
		"metric": 7.0 * narrow.get_char_size(48, 40).x,
	}
	expected["nested"] = expected["overlap"] + expected["wildcard"] + expected["base"]
	for id: String in expected:
		var actual: float = _rect(layout, id).size.x
		_check(is_equal_approx(actual, expected[id]), "Composite font %s width %s vs %s" % [id, actual, expected[id]])
	_check(is_equal_approx(_rect(layout, "ranged_height").size.x, _rect(layout, "reference_height").size.x),
		"A face that excludes space supplies glyphs but does not supply ex metrics")
	var plain: String = layout.get_plain_text()
	var start: int = plain.find("IMZ")
	_check(start >= 0 and layout.get_selection_rects(start, start + 3).size() > 0,
		"The mixed-font text retains its original indices and selection geometry")


func _check_oblique_angles() -> void:
	var document: HTMLDocument = load("{FONT_FACE_UID}")
	if document == null:
		return
	var assets: Dictionary = document.get("_local_fonts")
	var regular: FontFile = assets["res://fonts/times.ttf"]
	var narrow: FontFile = assets["res://fonts/cour.ttf"]
	var bold: FontFile = assets["res://fonts/arialbd.ttf"]
	var fixture: HTMLDocument = HTMLDocument.new()
	fixture.set("_local_fonts", assets)
	fixture.text = """<!doctype html><style>
		@font-face { font-family: Slanted; src: url(fonts/times.ttf); font-style: oblique 8deg 1deg; }
		@font-face { font-family: Slanted; src: url(fonts/cour.ttf); font-style: oblique 17deg 24deg; }
		@font-face { font-family: Slanted; src: url(fonts/arialbd.ttf); font-style: oblique -25deg -5deg;
			font-style: oblique -91deg; }
		@font-face { font-family: Upright; src: url(fonts/cour.ttf); }
		body { margin: 0; background: black; color: white; font: 40px Slanted; }
		.geometry { width: 7ch; height: 4px; }
		.ink { font: 48px/63px Upright; height: 63px; padding-left: 50px; }
		#weak { font-style: oblique 10deg; }
		#strong { font: oblique 12deg 40px Slanted; }
		#default { font-style: oblique; }
		#zero { font-style: oblique calc(20deg - 20deg); }
		#reverse { font-style: oblique calc(-100grad / 5); }
		#inside { font-style: var(--slope); --slope: oblique 0.05turn; }
		#invalid { font-style: oblique 5deg; font-style: oblique 91deg; }
		#right { font-style: oblique 32deg; }
		#left { font-style: oblique -32deg; }
	</style>
	<div id="weak" class="geometry"></div><div id="strong" class="geometry"></div>
	<div id="default" class="geometry"></div><div id="zero" class="geometry"></div>
	<div id="reverse" class="geometry"></div><div id="inside" class="geometry"></div>
	<div id="invalid" class="geometry"></div>
	<div id="right" class="ink">I</div><div id="upright" class="ink">I</div><div id="left" class="ink">I</div>"""
	var layout: HTMLLayout = fixture.create_layout()
	_check(layout.layout(Vector2i(300, 260)) == OK, "Lay out angled font styles")
	var expected: Dictionary[String, float] = {
		"weak": 7.0 * regular.get_char_size(48, 40).x,
		"strong": 7.0 * narrow.get_char_size(48, 40).x,
		"default": 7.0 * narrow.get_char_size(48, 40).x,
		"zero": 7.0 * regular.get_char_size(48, 40).x,
		"reverse": 7.0 * bold.get_char_size(48, 40).x,
		"inside": 7.0 * narrow.get_char_size(48, 40).x,
		"invalid": 7.0 * regular.get_char_size(48, 40).x,
	}
	for id: String in expected:
		var actual: float = _rect(layout, id).size.x
		_check(is_equal_approx(actual, expected[id]), "Oblique %s width %s vs %s" % [id, actual, expected[id]])

	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(300, 260)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(300.0, 260.0)
	label.document = fixture
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	var right: float = _ink_center_x(image, _label_rect(label, "right"))
	var upright: float = _ink_center_x(image, _label_rect(label, "upright"))
	var left: float = _ink_center_x(image, _label_rect(label, "left"))
	_check(right > upright + 3.0 and upright > left + 3.0,
		"Synthetic oblique applies the requested sign with independent font caches: %s, %s, %s" %
		[right, upright, left])
	viewport.queue_free()


func _check_font_width() -> void:
	var source: HTMLDocument = load("{FONT_FACE_UID}")
	if source == null:
		return
	var assets: Dictionary = source.get("_local_fonts")
	var regular: FontFile = assets["res://fonts/times.ttf"]
	var narrow: FontFile = assets["res://fonts/cour.ttf"]
	var bold: FontFile = assets["res://fonts/arialbd.ttf"]
	var variable: FontFile = assets["res://fonts/bahnschrift.ttf"]
	var fixture: HTMLDocument = HTMLDocument.new()
	fixture.set("_local_fonts", assets)
	fixture.text = """<!doctype html><style>
		@font-face { font-family: Widths; src: url(fonts/times.ttf); font-width: normal; }
		@font-face { font-family: Widths; src: url(fonts/cour.ttf); font-stretch: 80% 65%; }
		@font-face { font-family: Widths; src: url(fonts/arialbd.ttf); font-width: extra-expanded; }
		@font-face { font-family: Limits; src: url(fonts/bahnschrift.ttf); font-width: 90% 80%; }
		@font-face { font-family: Automatic; src: url(fonts/bahnschrift.ttf); font-width: auto; }
		body { margin: 0; font: 40px Widths; }
		div { width: 7ch; height: 5px; }
		#keyword { font-width: condensed; }
		#prefer_narrow { font-width: 98%; font-weight: bold; }
		#prefer_wide { font-width: 101%; }
		#alias { font-width: 100%; font-stretch: var(--measure); --measure: calc(50% + 25%); }
		#earlier_important { font-stretch: 75% !important; font-width: 100%; }
		#later_important { font-stretch: 75% !important; font-width: 100% !important; }
		#reset { font-stretch: 75%; font: 40px Widths; }
		#shorthand { font: oblique 12deg small-caps 700 condensed 40px Widths; }
		#inherit_parent { font-stretch: condensed; }
		#inherit { font-width: expanded; font-stretch: inherit; }
		#invalid { font-width: condensed; font-stretch: -10%; }
		#invalid_variable { font-stretch: var(--missing); }
		#limited_lower { font-family: Limits; font-width: 50%; }
		#limited_middle { font-family: Limits; font-stretch: 85%; }
		#limited_upper { font-family: Limits; font-width: 125%; }
		#automatic { font-family: Automatic; font-width: 75%; }
	</style>
	<div id="keyword"></div><div id="prefer_narrow"></div><div id="prefer_wide"></div>
	<div id="alias"></div><div id="earlier_important"></div><div id="later_important"></div>
	<div id="reset"></div><div id="shorthand"></div><section id="inherit_parent"><div id="inherit"></div></section>
	<div id="invalid"></div><div id="invalid_variable"></div>
	<div id="limited_lower"></div><div id="limited_middle"></div><div id="limited_upper"></div>
	<div id="automatic"></div>"""
	var layout: HTMLLayout = fixture.create_layout()
	layout.css_diagnostics_enabled = true
	_check(layout.layout(Vector2i(480, 280)) == OK, "Lay out font-width and its legacy alias")
	var plain_width: float = 7.0 * regular.get_char_size(48, 40).x
	var narrow_width: float = 7.0 * narrow.get_char_size(48, 40).x
	var expected: Dictionary[String, float] = {
		"keyword": narrow_width,
		"prefer_narrow": narrow_width,
		"prefer_wide": 7.0 * bold.get_char_size(48, 40).x,
		"alias": narrow_width,
		"earlier_important": narrow_width,
		"later_important": plain_width,
		"reset": plain_width,
		"shorthand": narrow_width,
		"inherit": narrow_width,
		"invalid": narrow_width,
		"invalid_variable": plain_width,
		"limited_lower": _variable_width_metric(variable, 80.0),
		"limited_middle": _variable_width_metric(variable, 85.0),
		"limited_upper": _variable_width_metric(variable, 90.0),
		"automatic": _variable_width_metric(variable, 75.0),
	}
	for id: String in expected:
		var actual: float = _rect(layout, id).size.x
		_check(is_equal_approx(actual, expected[id]), "Font width %s is %s vs %s" % [id, actual, expected[id]])
	_check(expected["limited_lower"] < expected["limited_upper"], "The test font exposes visible width changes")
	_check(_has_diagnostic(layout, "font-stretch", "-10%", "style_element"),
		"Invalid legacy values diagnose the original property name")
	_check(_has_diagnostic(layout, "font-stretch", "var(--missing)", "style_element"),
		"Invalid deferred legacy values retain the original property name")


func _variable_width_metric(font: FontFile, width: float) -> float:
	var variation: FontVariation = FontVariation.new()
	variation.base_font = font
	variation.variation_opentype = {0x77647468: width,}
	return 7.0 * variation.get_char_size(48, 40).x


func _check_opentype_settings() -> void:
	var source: HTMLDocument = load("{FONT_FACE_UID}")
	if source == null:
		return
	var assets: Dictionary = source.get("_local_fonts")
	var regular: FontFile = assets["res://fonts/times.ttf"]
	var variable: FontFile = assets["res://fonts/bahnschrift.ttf"]
	var fixture: HTMLDocument = HTMLDocument.new()
	fixture.set("_local_fonts", assets)
	fixture.text = """<!doctype html><style>
		@font-face { font-family: Regular; src: url(fonts/times.ttf); }
		@font-face { font-family: Quiet; src: url(fonts/times.ttf); font-feature-settings: "kern" off; }
		@font-face { font-family: Lead; src: url(fonts/cour.ttf); unicode-range: U+0058;
			font-feature-settings: "kern" off; }
		@font-face { font-family: Axes; src: url(fonts/bahnschrift.ttf); font-width: 85% 90%;
			font-variation-settings: "wdth" 84; font-variation-settings: "wdth"; }
		body { margin: 0; font: 40px Regular; }
		span { display: inline-block; }
		#unaffected, #invalid_literal, .plain_off { font-feature-settings: "kern" off; }
		#invalid_literal { font-feature-settings: kern; }
		#normal, #face_normal { font-feature-settings: normal; }
		#reset { font: var(--reset); --reset: 40px Regular; }
		#computed { font-feature-settings: var(--setting); --setting: "kern" off, "kern" calc(0.5), "zzzz" 2; }
		#face, #face_normal, #face_override { font-family: Quiet; }
		#face_override { font-feature-settings: "kern"; }
		#fallback { font-family: Lead, Regular; }
		#system { font-family: "Times New Roman"; font-feature-settings: "kern" 0; }
		.kerning_none { font-kerning: none; }
		#kerning_auto { font-kerning: auto; }
		#kerning_reset { font: 40px Regular; }
		#kerning_var { font-kerning: var(--kerning); --kerning: none; }
		#kerning_face { font-family: Quiet; font-kerning: normal; }
		#kerning_explicit { font-kerning: normal; font-feature-settings: "kern" 0; }
		.axis { font-family: Axes; width: 7ch; height: 5px; }
		#axis_override { font-variation-settings: "wdth" 100, "wdth" calc(150 / 2), "ZZZZ" -1.5; }
		.axis_parent { font-variation-settings: "wdth" 75; }
		#axis_normal { font-variation-settings: normal; }
		#axis_invalid { font-variation-settings: var(--missing); }
		#axis_clamp { font-variation-settings: "wdth" 200; }
		#axis_reset { font-variation-settings: "wdth" 75; font: 40px Axes; }
		#support { width: 10px; height: 5px; }
		@supports (font-feature-settings: "kern" 0) and (font-variation-settings: "wdth" 80.5) {
			#support { width: 17px; }
		}
	</style>
	<div><span id="default">AVAV</span><span id="unaffected">AVAV</span></div>
	<div class="plain_off"><span id="inherit">AVAV</span><span id="normal">AVAV</span>
		<span id="reset">AVAV</span><span id="computed">AVAV</span><span id="invalid_literal">AVAV</span></div>
	<div><span id="face">AVAV</span><span id="face_normal">AVAV</span>
		<span id="face_override">AVAV</span><span id="system">AVAV</span><span id="fallback">AVAV</span></div>
	<div class="kerning_none"><span id="kerning_inherit">AVAV</span><span id="kerning_auto">AVAV</span>
		<span id="kerning_reset">AVAV</span><span id="kerning_var">AVAV</span>
		<span id="kerning_face">AVAV</span><span id="kerning_explicit">AVAV</span></div>
	<div class="axis" id="axis_default"></div><div class="axis" id="axis_override"></div>
	<div class="axis" id="axis_invalid"></div><div class="axis" id="axis_clamp"></div>
	<div class="axis" id="axis_reset"></div>
	<section class="axis_parent"><div class="axis" id="axis_inherit"></div><div class="axis" id="axis_normal"></div></section>
	<div id="support"></div>"""
	var layout: HTMLLayout = fixture.create_layout()
	layout.css_diagnostics_enabled = true
	_check(layout.layout(Vector2i(700, 260)) == OK, "Lay out OpenType feature and variation settings")
	var disabled: FontVariation = FontVariation.new()
	disabled.base_font = regular
	disabled.opentype_features = {0x6b65726e: 0,}
	var with_kerning: float = regular.get_string_size("AVAV", HORIZONTAL_ALIGNMENT_LEFT, -1.0, 40).x
	var without_kerning: float = disabled.get_string_size("AVAV", HORIZONTAL_ALIGNMENT_LEFT, -1.0, 40).x
	_check(without_kerning - with_kerning > 1.0, "The fixture font changes advances when kerning is disabled")
	for id: String in [
		"default", "normal", "reset", "computed", "face_override", "fallback",
		"kerning_auto", "kerning_reset", "kerning_face",
	]:
		var actual: float = _rect(layout, id).size.x
		_check(absf(actual - with_kerning) <= 1.0, "%s uses kerning: %s vs %s" % [id, actual, with_kerning])
	for id: String in [
		"unaffected", "inherit", "invalid_literal", "face", "face_normal", "system",
		"kerning_inherit", "kerning_var", "kerning_explicit",
	]:
		var actual: float = _rect(layout, id).size.x
		_check(absf(actual - without_kerning) <= 1.0, "%s disables kerning: %s vs %s" % [id, actual, without_kerning])
	var widths: Dictionary[String, float] = {
		"axis_default": 84.0,
		"axis_override": 75.0,
		"axis_invalid": 84.0,
		"axis_clamp": 100.0,
		"axis_reset": 84.0,
		"axis_inherit": 75.0,
		"axis_normal": 84.0,
	}
	for id: String in widths:
		var expected: float = _variable_width_metric(variable, widths[id])
		var actual: float = _rect(layout, id).size.x
		_check(is_equal_approx(actual, expected), "OpenType axis %s is %s vs %s" % [id, actual, expected])
	_check(is_equal_approx(_rect(layout, "support").size.x, 17.0), "@supports recognizes both settings")
	_check(_has_diagnostic(layout, "font-feature-settings", "kern", "style_element"), "Unquoted tags are invalid")
	_check(_has_diagnostic(layout, "font-variation-settings", "var(--missing)", "style_element"),
		"Invalid variables in axis settings retain their original declaration")


func _check_font_variants() -> void:
	const FONT_UID: String = "{VARIANT_FONT_UID}"
	const LIGATURE_SAMPLE: String = "officeaffliction"
	const NUMERIC_SAMPLE: String = "1111890"
	const CAPS_SAMPLE: String = "Titlecase"
	const COMBINED_SAMPLE: String = LIGATURE_SAMPLE + NUMERIC_SAMPLE
	if FONT_UID.is_empty():
		print("Skipping Latin font variant fixture because Calibri is unavailable")
		return
	var regular: FontFile = load(FONT_UID)
	_check(regular != null, "Load the Latin font variant fixture")
	if regular == null:
		return
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_local_fonts", {"res://fonts/variant.ttf": regular,})
	document.text = """<!doctype html><style>
		@font-face { font-family: Probe; src: url(fonts/variant.ttf); }
		@font-face { font-family: Quiet; src: url(fonts/variant.ttf); font-feature-settings: "liga" 0; }
		body { margin: 0; font: 48px Probe; }
		span { display: inline-block; }
		.ligatures-off { font-variant-ligatures: no-common-ligatures; }
		#ligature_reset { font: 48px Probe; }
		#ligature_override { font-feature-settings: "liga" on; }
		#face_initial, #face_override { font-family: Quiet; }
		#face_override { font-variant-ligatures: common-ligatures; }
		#numeric { font-variant-numeric: proportional-nums; }
		#conflict { font-variant-numeric: proportional-nums; font-variant-numeric: tabular-nums proportional-nums; }
		#combined { --forms: no-common-ligatures tabular-nums; font-variant: var(--forms);
			font-variant-numeric: proportional-nums; }
		#caps { font-variant-caps: all-small-caps; }
		#petite { font-variant-caps: all-petite-caps; }
		#legacy { font: small-caps 48px Probe; }
		.numeric { font-variant-numeric: proportional-nums; }
		#none { font-variant: none; }
		#none_explicit { font-variant: none; font-feature-settings: "liga" on; }
		#tracked, #tracked_ligature { letter-spacing: 3px; }
		#tracked_ligature { font-feature-settings: "liga" on; }
		#support { width: 19px; height: 4px; }
		@supports (font-variant: small-caps tabular-nums historical-ligatures) and
			(font-variant-numeric: slashed-zero oldstyle-nums ordinal stacked-fractions proportional-nums) and
			(not (font-variant-ligatures: normal no-common-ligatures)) {
			#support { width: 41px; }
		}
	</style>
	<div><span id="ligature_initial">{ligatures}</span></div>
	<div class="ligatures-off"><span id="ligature_inherit">{ligatures}</span></div>
	<div class="ligatures-off"><span id="ligature_reset">{ligatures}</span></div>
	<div class="ligatures-off"><span id="ligature_override">{ligatures}</span></div>
	<div><span id="face_initial">{ligatures}</span><span id="face_override">{ligatures}</span></div>
	<div><span id="numeric">{numeric}</span><span id="conflict">{numeric}</span></div>
	<div><span id="combined">{combined}</span></div>
	<div><span id="caps">{caps}</span><span id="petite">{caps}</span><span id="legacy">{caps}</span></div>
	<div class="numeric"><span id="none">{combined}</span></div>
	<div class="numeric"><span id="none_explicit">{combined}</span></div><div id="support"></div>
	<div><span id="tracked">{ligatures}</span><span id="tracked_ligature">{ligatures}</span></div>
	""".replace("{ligatures}", LIGATURE_SAMPLE).replace("{numeric}", NUMERIC_SAMPLE) \
		.replace("{caps}", CAPS_SAMPLE).replace("{combined}", COMBINED_SAMPLE)
	var layout: HTMLLayout = document.create_layout()
	layout.css_diagnostics_enabled = true
	_check(layout.layout(Vector2i(950, 850)) == OK, "Lay out high-level OpenType feature properties")
	var ligatures: float = _feature_text_width(regular, LIGATURE_SAMPLE, {})
	var no_ligatures: float = _feature_text_width(regular, LIGATURE_SAMPLE, {"liga": 0, "clig": 0,})
	var ligature_line: TextLine = TextLine.new()
	ligature_line.add_string(LIGATURE_SAMPLE, regular, 48)
	var text_server: TextServer = TextServerManager.get_primary_interface()
	var ligature_clusters: int = text_server.shaped_text_get_glyph_count(ligature_line.get_rid())
	var proportional: float = _feature_text_width(regular, NUMERIC_SAMPLE, {"pnum": 1,})
	var small_caps: float = _feature_text_width(regular, CAPS_SAMPLE, {"smcp": 1, "c2sc": 1,})
	_check(absf(ligatures - no_ligatures) > 0.1, "The fixture has measurable common ligatures")
	_check(absf(proportional - _feature_text_width(regular, NUMERIC_SAMPLE, {})) > 0.1,
		"The fixture has measurable proportional digits")
	_check(absf(small_caps - _feature_text_width(regular, CAPS_SAMPLE, {})) > 0.1,
		"The fixture has measurable small capitals")
	var expected: Dictionary[String, float] = {
		"ligature_initial": ligatures,
		"ligature_inherit": no_ligatures,
		"ligature_reset": ligatures,
		"ligature_override": ligatures,
		"face_initial": no_ligatures,
		"face_override": ligatures,
		"numeric": proportional,
		"conflict": proportional,
		"combined": _feature_text_width(regular, COMBINED_SAMPLE, {"liga": 0, "clig": 0, "pnum": 1,}),
		"caps": small_caps,
		"petite": small_caps,
		"legacy": _feature_text_width(regular, CAPS_SAMPLE, {"smcp": 1,}),
		"none": _feature_text_width(regular, COMBINED_SAMPLE, {"liga": 0, "clig": 0, "calt": 0, "dlig": 0, "hlig": 0,}),
		"none_explicit": _feature_text_width(regular, COMBINED_SAMPLE,
			{"liga": 1, "clig": 0, "calt": 0, "dlig": 0, "hlig": 0,}),
		"support": 41.0,
		"tracked": no_ligatures + float(LIGATURE_SAMPLE.length() - 1) * 3.0,
		"tracked_ligature": ligatures + float(ligature_clusters - 1) * 3.0,
	}
	for id: String in expected:
		var width: float = _rect(layout, id).size.x
		_check(absf(width - expected[id]) <= 0.1, "Font variant %s is %s vs %s" % [id, width, expected[id]])
	_check(_has_diagnostic(layout, "font-variant-numeric", "tabular-nums proportional-nums", "style_element"),
		"Conflicting numeric alternatives are rejected atomically")


func _check_named_font_alternates() -> void:
	const FONT_UID: String = "{VARIANT_FONT_UID}"
	const SET_SAMPLE: String = "Monogram"
	const STYLISTIC_SAMPLE: String = "QwertyAmpersand&"
	if FONT_UID.is_empty():
		print("Skipping named font alternates because Calibri is unavailable")
		return
	var font: FontFile = load(FONT_UID)
	_check(font != null, "Load the named font alternates fixture")
	if font == null:
		return
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_local_fonts", {"res://fonts/variant.ttf": font,})
	document.text = """<!doctype html><style>
		@import "styles/font-alternates.css";
		@layer base, theme;
		@font-face { font-family: Probe; src: url(fonts/variant.ttf); }
		@font-face { font-family: Twin; src: url(fonts/variant.ttf); }
		@font-face { font-family: Unmapped; src: url(fonts/variant.ttf); }
		@font-face { font-family: Partial; src: url(fonts/variant.ttf); unicode-range: U+0058; }
		@layer theme { @font-feature-values "pRoBe" { @styleset { chosen: 4; } } }
		@layer base { @font-feature-values Probe { @styleset { chosen: 1; } } }
		@font-feature-values Probe, Twin {
			@styleset { CaseName: 4; combined: 1 4; noop: 4; responsive: 1; valid: 4; disabled: 4; }
			@stylistic { custom: 1; custom: 1 2; }
			@unknown { ignored: 4; }
		}
		@font-feature-values probe { @styleset { noop: 0 100 125; valid: -1; } }
		@font-feature-values serif, Probe { @styleset { invalid-family-list: 4; } }
		@media (min-width: 800px) { @font-feature-values Probe { @styleset { responsive: 4; } } }
		body { margin: 0; font: 48px Probe; }
		span { display: inline-block; }
		#layered, .inherited { font-variant-alternates: styleset(chosen); }
		#case_name { font-variant-alternates: styleset(CaseName); }
		#case_mismatch { font-variant-alternates: styleset(casename); }
		#family_list { font-family: Twin; font-variant-alternates: styleset(combined); }
		#unmapped { font-family: Unmapped; font-variant-alternates: styleset(CaseName); }
		#fallback { font-family: Partial, Probe; font-variant-alternates: styleset(chosen); }
		#many { font-variant-alternates: styleset(imported, combined, unknown); }
		#imported { font-variant-alternates: styleset(imported); }
		#noop { font-variant-alternates: styleset(noop); }
		#bad_family { font-variant-alternates: styleset(invalid-family-list); }
		#bad_definition { font-variant-alternates: styleset(valid); }
		#responsive { font-variant-alternates: styleset(responsive); }
		#overridden { font-variant-alternates: styleset(disabled); font-feature-settings: "ss04" 0; }
		#stylistic { font-variant-alternates: stylistic(custom); }
		#shorthand { --v: tabular-nums styleset(chosen); font-variant: var(--v); }
		#font_reset { font: 48px Probe; }
		#variant_reset { font-variant: normal; }
		#invalid { font-variant-alternates: styleset(chosen); font-variant-alternates: styleset(chosen) styleset(noop); }
		#support { width: 19px; height: 4px; }
		@supports at-rule(@font-feature-values) and
			(font-variant-alternates: historical-forms character-variant(First, Other) swash(S) ornaments(O) annotation(A)) and
			(not (font-variant-alternates: stylistic(first, second))) {
			#support { width: 53px; }
		}
	</style>
	<div><span id="base">{set}</span><span id="layered">{set}</span></div>
	<div><span id="case_name">{set}</span><span id="case_mismatch">{set}</span></div>
	<div><span id="family_list">{set}</span><span id="unmapped">{set}</span></div>
	<div><span id="fallback">{set}</span><span id="many">{set}</span><span id="noop">{set}</span></div>
	<div><span id="imported">{set}</span></div>
	<div><span id="bad_family">{set}</span><span id="bad_definition">{set}</span></div>
	<div><span id="responsive">{set}</span><span id="overridden">{set}</span></div>
	<div><span id="stylistic">{stylistic}</span></div>
	<div><span id="shorthand">{set}</span><span id="invalid">{set}</span></div>
	<div class="inherited"><span id="inherit">{set}</span><span id="font_reset">{set}</span>
		<span id="variant_reset">{set}</span></div><div id="support"></div>
	""".replace("{set}", SET_SAMPLE).replace("{stylistic}", STYLISTIC_SAMPLE.xml_escape())
	var layout: HTMLLayout = document.create_layout()
	_check(layout.layout(Vector2i(760, 850)) == OK, "Lay out named OpenType alternates")
	var base: float = _feature_text_width(font, SET_SAMPLE, {})
	var ss04: float = _feature_text_width(font, SET_SAMPLE, {"ss04": 1,})
	var salt: float = _feature_text_width(font, STYLISTIC_SAMPLE, {"salt": 1,})
	_check(absf(base - ss04) > 0.1, "Fixture provides a measurable stylistic set")
	_check(absf(salt - _feature_text_width(font, STYLISTIC_SAMPLE, {})) > 1.0, "Fixture provides a measurable stylistic alternate")
	var expected: Dictionary[String, float] = {
		"base": base,
		"layered": ss04,
		"case_name": ss04,
		"case_mismatch": base,
		"family_list": _feature_text_width(font, SET_SAMPLE, {"ss01": 1, "ss04": 1,}),
		"unmapped": base,
		"fallback": ss04,
		"many": _feature_text_width(font, SET_SAMPLE, {"ss01": 1, "ss04": 1,}),
		"imported": ss04,
		"noop": base,
		"bad_family": base,
		"bad_definition": ss04,
		"responsive": _feature_text_width(font, SET_SAMPLE, {"ss01": 1,}),
		"overridden": base,
		"stylistic": salt,
		"shorthand": ss04,
		"invalid": ss04,
		"inherit": ss04,
		"font_reset": base,
		"variant_reset": base,
		"support": 53.0,
	}
	for id: String in expected:
		var width: float = _rect(layout, id).size.x
		_check(absf(width - expected[id]) <= 0.1, "Named font alternate %s is %s vs %s" % [id, width, expected[id]])
	_check(layout.layout(Vector2i(860, 850)) == OK and absf(_rect(layout, "responsive").size.x - ss04) <= 0.1,
		"Activating a conditional definition refreshes text shaping")
	_check(layout.layout(Vector2i(760, 850)) == OK and absf(_rect(layout, "responsive").size.x - expected["responsive"]) <= 0.1,
		"Removing a conditional definition restores the earlier feature value")


func _check_historical_forms() -> void:
	const FONT_UID: String = "{HISTORICAL_FONT_UID}"
	const SAMPLE: String = "QwertyAmpersand&"
	if FONT_UID.is_empty():
		print("Skipping historical forms because Palatino Linotype is unavailable")
		return
	var font: FontFile = load(FONT_UID)
	_check(font != null, "Load the historical forms fixture")
	if font == null:
		return
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_local_fonts", {"res://fonts/historical.ttf": font,})
	document.text = """<!doctype html><style>
		@font-face { font-family: History; src: url(fonts/historical.ttf); }
		body { margin: 0; font: 48px History; }
		span { display: inline-block; }
		#historical { font-variant-alternates: historical-forms; }
	</style><span id="historical">%s</span>""" % SAMPLE.xml_escape()
	var layout: HTMLLayout = document.create_layout()
	_check(layout.layout(Vector2i(640, 360)) == OK, "Lay out historical forms")
	var expected: float = _feature_text_width(font, SAMPLE, {"hist": 1,})
	_check(absf(expected - _feature_text_width(font, SAMPLE, {})) > 1.0, "Fixture provides measurable historical forms")
	_check(absf(_rect(layout, "historical").size.x - expected) <= 0.1, "Historical forms work without a named definition")


func _check_east_asian_variants() -> void:
	const FONT_UID: String = "{EAST_ASIAN_FONT_UID}"
	const SAMPLE: String = "AA11"
	if FONT_UID.is_empty():
		print("Skipping East Asian font fixture because YuGothM.ttc is unavailable")
		return
	var font: FontFile = load(FONT_UID)
	_check(font != null and font.get_supported_feature_list().has(0x66776964), "Fixture font provides full-width forms")
	if font == null:
		return
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_local_fonts", {"res://fonts/east_asian.ttc": font,})
	document.text = """<!doctype html><style>
		@font-face { font-family: Eastern; src: url(fonts/east_asian.ttc); }
		body { margin: 0; font: 48px Eastern; }
		span { display: inline-block; }
		#full, #overridden { font-variant-east-asian: jis04 full-width ruby; }
		#overridden { font-feature-settings: "fwid" off; }
		#shorthand { font-variant: full-width lining-nums; }
		#support { width: 19px; height: 4px; }
		@supports (font-variant-east-asian: traditional proportional-width ruby) and
			(not (font-variant-east-asian: simplified traditional)) {
			#support { width: 43px; }
		}
	</style>
	<div><span id="full">{sample}</span><span id="overridden">{sample}</span></div>
	<div><span id="shorthand">{sample}</span></div><div id="support"></div>
	""".replace("{sample}", SAMPLE)
	var layout: HTMLLayout = document.create_layout()
	_check(layout.layout(Vector2i(540, 380)) == OK, "Lay out East Asian variants")
	var full: float = _feature_text_width(font, SAMPLE, {"fwid": 1, "jp04": 1, "ruby": 1,})
	var overridden: float = _feature_text_width(font, SAMPLE, {"fwid": 0, "jp04": 1, "ruby": 1,})
	_check(absf(full - overridden) > 1.0, "The fixture has measurable East Asian width forms")
	_check(absf(_rect(layout, "full").size.x - full) <= 0.1, "East Asian options combine across categories")
	_check(absf(_rect(layout, "overridden").size.x - overridden) <= 0.1, "Explicit features override East Asian forms")
	var shorthand: float = _feature_text_width(font, SAMPLE, {"fwid": 1, "lnum": 1,})
	_check(absf(_rect(layout, "shorthand").size.x - shorthand) <= 0.1, "Shorthand accepts East Asian and numeric forms")
	_check(is_equal_approx(_rect(layout, "support").size.x, 43.0), "East Asian categories reject conflicting alternatives")


func _feature_text_width(font: Font, sample: String, features: Dictionary) -> float:
	var variation: FontVariation = FontVariation.new()
	variation.base_font = font
	variation.opentype_features = features
	return variation.get_string_size(sample, HORIZONTAL_ALIGNMENT_LEFT, -1.0, 48).x


func _check_optical_sizing() -> void:
	const FONT_UID: String = "{OPTICAL_FONT_UID}"
	const OPTICAL_AXIS: int = 0x6f70737a
	const SAMPLE: String = "MMMMmmmmminimum"
	if FONT_UID.is_empty():
		print("Skipping optical sizing fixture because SegUIVar.ttf is unavailable")
		return
	var font: FontFile = load(FONT_UID)
	_check(font != null and font.get_supported_variation_list().has(OPTICAL_AXIS), "Fixture font exposes the opsz axis")
	if font == null or not font.get_supported_variation_list().has(OPTICAL_AXIS):
		return
	var fixture: HTMLDocument = HTMLDocument.new()
	fixture.set("_local_fonts", {"res://fonts/optical.ttf": font,})
	fixture.text = """<!doctype html><style>
		@font-face { font-family: Optical; src: url(fonts/optical.ttf); }
		@font-face { font-family: FixedOptical; src: url(fonts/optical.ttf); font-variation-settings: "opsz" 8; }
		body { margin: 0; font: 32px Optical; }
		span { display: inline-block; }
		#small, #responsive { font-size: 16px; }
		#large { font-size: 64px; }
		.off { font-optical-sizing: var(--optical); --optical: none; }
		#invalid { font-optical-sizing: normal; }
		#reset { font: 32px Optical; }
		#explicit { font-variation-settings: "opsz" 31; }
		#descriptor, #descriptor_auto, #descriptor_override { font-family: FixedOptical; }
		#descriptor_override { font-variation-settings: "opsz" 31; }
		@media (min-width: 800px) { #responsive { font-size: 32px; } }
	</style>
	<div><span id="small">{sample}</span></div><div><span id="auto">{sample}</span></div>
	<div><span id="large">{sample}</span></div><div><span id="responsive">{sample}</span></div>
	<div class="off"><span id="none">{sample}</span></div><div class="off"><span id="invalid">{sample}</span></div>
	<div class="off"><span id="reset">{sample}</span></div><div class="off"><span id="explicit">{sample}</span></div>
	<div class="off"><span id="descriptor">{sample}</span></div><div><span id="descriptor_auto">{sample}</span></div>
	<div><span id="descriptor_override">{sample}</span></div>
	""".replace("{sample}", SAMPLE)
	var layout: HTMLLayout = fixture.create_layout()
	layout.css_diagnostics_enabled = true
	_check(layout.layout(Vector2i(760, 540)) == OK, "Lay out automatic and explicit optical sizing")
	var baseline: float = font.get_string_size(SAMPLE, HORIZONTAL_ALIGNMENT_LEFT, -1.0, 32).x
	var automatic: float = _optical_text_width(font, SAMPLE, 32, 24.0)
	_check(absf(baseline - automatic) > 1.0, "Optical sizing changes actual glyph advances")
	var expected: Dictionary[String, float] = {
		"small": _optical_text_width(font, SAMPLE, 16, 12.0),
		"auto": automatic,
		"large": _optical_text_width(font, SAMPLE, 64, 36.0),
		"responsive": _optical_text_width(font, SAMPLE, 16, 12.0),
		"none": baseline,
		"invalid": baseline,
		"reset": automatic,
		"explicit": _optical_text_width(font, SAMPLE, 32, 31.0),
		"descriptor": _optical_text_width(font, SAMPLE, 32, 8.0),
		"descriptor_auto": automatic,
		"descriptor_override": _optical_text_width(font, SAMPLE, 32, 31.0),
	}
	for id: String in expected:
		var actual: float = _rect(layout, id).size.x
		_check(absf(actual - expected[id]) <= 1.0, "Optical size %s is %s vs %s" % [id, actual, expected[id]])
	_check(_has_diagnostic(layout, "font-optical-sizing", "normal", "style_element"), "Optical sizing reports invalid values")
	_check(layout.layout(Vector2i(880, 540)) == OK and absf(_rect(layout, "responsive").size.x - automatic) <= 1.0,
		"Media changes select the optical size for the new font size")
	var installed: SystemFont = SystemFont.new()
	installed.font_names = PackedStringArray(["Segoe UI Variable",])
	if installed.get_supported_variation_list().has(OPTICAL_AXIS):
		var system_layout: HTMLLayout = HTMLLayout.new()
		var html: String = """<!doctype html><span id="system" style="display: inline-block;
			font: 32px 'Segoe UI Variable'">%s</span>""" % SAMPLE
		var system_width: float = _optical_text_width(installed, SAMPLE, 32, 24.0)
		_check(system_layout.parse(html) == OK and system_layout.layout(Vector2i(760, 540)) == OK and
			absf(_rect(system_layout, "system").size.x - system_width) <= 1.0, "Installed fonts use automatic sizing")


func _optical_text_width(font: Font, sample: String, size: int, optical_size: float) -> float:
	var variation: FontVariation = FontVariation.new()
	variation.base_font = font
	variation.variation_opentype = {0x6f70737a: optical_size,}
	return variation.get_string_size(sample, HORIZONTAL_ALIGNMENT_LEFT, -1.0, size).x


func _check_text_spacing() -> void:
	var source: HTMLDocument = load("{FONT_FACE_UID}")
	if source == null:
		return
	var assets: Dictionary = source.get("_local_fonts")
	var font: FontFile = assets["res://fonts/cour.ttf"]
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_local_fonts", assets)
	document.text = """<!doctype html><style>
		@font-face { font-family: Mono; src: url(fonts/cour.ttf); }
		body { margin: 0; font: 20px / 28px Mono; }
		.probe { display: inline-block; }
		#letter, #selected { letter-spacing: 2.5px; }
		#negative { letter-spacing: -2.5px; }
		#separators, #nbsp, #fixed { word-spacing: 3.25px; }
		#negative_words { word-spacing: -3.25px; }
		#ascii { letter-spacing: 3.25px; word-spacing: 3.25px; }
		.inherited { letter-spacing: calc(10% + 1px); word-spacing: 0.25em; }
		.big { font-size: 40px; }
		#em_parent { letter-spacing: 0.15em; }
		#outside { letter-spacing: 2px; }
		#inside { letter-spacing: 6px; }
		.wrap { width: 41px; }
		#wrap, #wrap_closed { word-spacing: 8px; }
		#cursive { letter-spacing: 7px; }
		#invalid { letter-spacing: 2.5px; letter-spacing: auto; }
		#variable { --spacing: 2.5px; letter-spacing: var(--spacing); }
	</style>
	<div><span class="probe" id="letter">ABCD</span><span class="probe" id="negative">ABCD</span></div>
	<div><span class="probe" id="separators">A    B C   </span></div>
	<div><span class="probe" id="negative_words">A B C</span></div>
	<div><span class="probe" id="nbsp">A&nbsp;B</span><span class="probe" id="ascii">A B</span></div>
	<div><span class="probe" id="fixed">A&#x3000;B</span><span class="probe" id="fixed_base">A&#x3000;B</span></div>
	<div class="inherited"><span class="probe big" id="percent">A B</span></div>
	<div id="em_parent"><span class="probe big" id="em">AB</span></div>
	<div><span class="probe" id="outside"><span id="left">M</span><span id="inside">N</span></span></div>
	<div class="wrap" id="wrap">A B</div><div class="wrap" id="wrap_base">A B</div>
	<div class="wrap" id="wrap_closed"><span id="wrap_first">A </span><span>B</span></div>
	<div><span class="probe" id="cursive">سلام</span><span class="probe" id="cursive_base">سلام</span></div>
	<div><span class="probe" id="invalid">ABCD</span><span class="probe" id="variable">ABCD</span></div>
	<div><span class="probe" id="selected">WXYZ</span></div>
	"""
	var layout: HTMLLayout = document.create_layout()
	layout.css_diagnostics_enabled = true
	_check(layout.layout(Vector2i(620, 650)) == OK, "Lay out fractional letter and word spacing")
	var unit: float = font.get_char_size(65, 20).x
	var large_unit: float = font.get_char_size(65, 40).x
	var expected: Dictionary[String, float] = {
		"letter": unit * 4.0 + 7.5,
		"negative": unit * 4.0 - 7.5,
		"separators": unit * 5.0 + 6.5,
		"negative_words": unit * 5.0 - 6.5,
		"nbsp": unit * 3.0 + 3.25,
		"ascii": unit * 3.0 + 9.75,
		"fixed": _rect(layout, "fixed_base").size.x,
		"percent": large_unit * 3.0 + 15.0,
		"em": large_unit * 2.0 + 3.0,
		"outside": unit * 2.0 + 4.0,
		"cursive": _rect(layout, "cursive_base").size.x,
		"invalid": unit * 4.0 + 7.5,
		"variable": unit * 4.0 + 7.5,
	}
	for id: String in expected:
		var width: float = _rect(layout, id).size.x
		_check(absf(width - expected[id]) <= 0.1, "Text spacing %s is %s vs %s" % [id, width, expected[id]])
	var left: Rect2 = _rect(layout, "left")
	var right: Rect2 = _rect(layout, "inside")
	var adjacent: int = layout.get_plain_text().find("MN")
	var adjacent_rects: Array[Rect2] = layout.get_selection_rects(adjacent, adjacent + 2)
	_check(adjacent_rects.size() == 2 and absf(adjacent_rects[1].position.x - adjacent_rects[0].end.x - 4.0) <= 0.1,
		"Adjacent elements share half their letter spacing")
	_check(absf(left.size.x - unit - 1.0) <= 0.1 and absf(right.size.x - unit - 3.0) <= 0.1,
		"Inline boxes include their own half of the shared spacing")
	_check(is_equal_approx(_rect(layout, "wrap").size.y, 56.0) and is_equal_approx(_rect(layout, "wrap_base").size.y, 28.0),
		"Word spacing participates in line breaking")
	_check(is_equal_approx(_rect(layout, "wrap_first").size.x, unit), "A wrapped line trims spaces before inline end markers")
	_check(_has_diagnostic(layout, "letter-spacing", "auto", "style_element"), "Invalid spacing preserves the earlier declaration")
	var offset: int = layout.get_plain_text().find("WXYZ")
	var selected: Rect2 = _rect(layout, "selected")
	var highlights: Array[Rect2] = layout.get_selection_rects(offset, offset + 4)
	_check(offset >= 0 and highlights.size() == 1 and absf(highlights[0].size.x - selected.size.x) <= 0.1,
		"Selection uses the same spaced width as layout")
	var caret: Vector2 = Vector2(selected.position.x + unit * 2.0 + 3.75, selected.get_center().y)
	_check(layout.hit_test_text(caret) == offset + 2, "Hit testing maps spaced glyphs back to the DOM text")
	await _check_text_spacing_paint(assets)


func _check_text_autospace() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var html: String = """
		<!doctype html><style>
			body { margin: 0; font-size: 32px; line-height: 40px; }
			span.probe { display: inline-block; }
			.none { text-autospace: no-autospace; }
			.alpha { text-autospace: var(--spacing); --spacing: ideograph-alpha; }
			.numeric { text-autospace: ideograph-numeric; }
			.replace { text-autospace: replace; }
			.letter { letter-spacing: 5px; }
			.word { word-spacing: 7px; }
			.french { text-autospace: punctuation; }
			.french.replace { text-autospace: replace punctuation; }
			#second { text-autospace: no-autospace; }
			#edge_left { padding-right: 7px; }
		</style>
		<div><span id="ic" class="probe">水</span></div>
		<div><span id="none" class="probe none">水A水1</span></div>
		<div><span id="normal" class="probe">水A水1</span></div>
		<div><span id="alpha" class="probe alpha">水A水1</span></div>
		<div><span id="numeric" class="probe numeric">水A水1</span></div>
		<div><span id="fullwidth" class="probe">水Ａ水１</span></div>
		<div><span id="fullwidth_ref" class="probe none">水Ａ水１</span></div>
		<div><span id="letters" class="probe letter">水A水1</span></div>
		<div><span id="letters_ref" class="probe letter none">水A水1</span></div>
		<div><span id="replace" class="probe replace">水 R</span></div>
		<div><span id="replace_ref" class="probe">水R</span></div>
		<div><span id="words" class="probe replace word">水    W</span></div>
		<div><span id="words_ref" class="probe">水W</span></div>
		<div><span id="insert" class="probe" style="text-autospace: insert">水 R</span></div>
		<div><span id="insert_ref" class="probe none">水 R</span></div>
		<div><span id="nbsp" class="probe replace">水&nbsp;R</span></div>
		<div><span id="nbsp_ref" class="probe none">水&nbsp;R</span></div>
		<div lang="fr-CA"><span id="french" class="probe french">Bonjour!</span></div>
		<div lang="fr"><span id="fr_replace" class="probe french replace">Bonjour !</span></div>
		<div><span id="fr_ref" class="probe none">Bonjour&#8239;!</span></div>
		<div lang="fr"><span id="quotes" class="probe french">«Mot»</span></div>
		<div><span id="quotes_ref" class="probe none">«&nbsp;Mot&nbsp;»</span></div>
		<div lang="en"><span id="english" class="probe french">Bonjour!</span></div>
		<div><span id="english_ref" class="probe none">Bonjour!</span></div>
		<div id="resize"><a id="first" href="#link">水</a><span id="second">A</span></div>
		<div id="fr_wrap" lang="fr" class="french replace"><span id="fr_word">Bonjour</span> <span id="fr_mark">!</span></div>
		<div><span id="edge_left">水</span><span id="edge_right">A</span></div>
		<div><span id="invalid_autospace" class="probe alpha"
			style="text-autospace: ideograph-alpha ideograph-alpha">水A水1</span></div>
	"""
	_check(layout.parse(html) == OK and layout.layout(Vector2i(480, 1200)) == OK, "Lay out CSS text-autospace probes")
	var ic: float = _rect(layout, "ic").size.x
	var step: float = ic / 8.0
	var unspaced: float = _rect(layout, "none").size.x
	var expected: Dictionary[String, float] = {
		"normal": unspaced + 3.0 * step,
		"alpha": unspaced + 2.0 * step,
		"numeric": unspaced + step,
		"invalid_autospace": unspaced + 2.0 * step,
		"fullwidth": _rect(layout, "fullwidth_ref").size.x,
		"letters": _rect(layout, "letters_ref").size.x + 3.0 * step,
		"replace": _rect(layout, "replace_ref").size.x,
		"words": _rect(layout, "words_ref").size.x + 7.0,
		"insert": _rect(layout, "insert_ref").size.x,
		"nbsp": _rect(layout, "nbsp_ref").size.x,
		"french": _rect(layout, "fr_ref").size.x,
		"fr_replace": _rect(layout, "fr_ref").size.x,
		"quotes": _rect(layout, "quotes_ref").size.x,
		"english": _rect(layout, "english_ref").size.x,
	}
	for id: String in expected:
		var width: float = _rect(layout, id).size.x
		_check(absf(width - expected[id]) < 0.15, "Autospace %s width %s vs %s" % [id, width, expected[id]])
	var plain: String = layout.get_plain_text()
	_check("水 W" in plain, "A replaced sequence of collapsible spaces copies as one ordinary space")
	var authored_space: int = plain.find("水 R") + 1
	var selected: Array[Rect2] = layout.get_selection_rects(authored_space, authored_space + 1)
	_check(authored_space > 0 and selected.size() == 1 and absf(selected[0].size.x - step) < 0.15,
		"An authored space stays selectable at its replacement width")
	_check("Bonjour\u202f!" in plain and "«Mot»" in plain and "Bonjour !" in plain and "Bonjour!" in plain,
		"Autospace preserves authored spaces and keeps inserted spacing out of copied text")
	var first: Rect2 = _rect(layout, "first")
	var second: Rect2 = _rect(layout, "second")
	var gap_point: Vector2 = Vector2(first.end.x + step / 2.0, first.get_center().y)
	_check(absf(second.position.x - first.end.x - step) < 0.15 and layout.get_link_at(gap_point).is_empty()
		and layout.get_link_at(first.get_center()) == "#link", "The shared boundary gap lies outside the link")
	_check(_rect(layout, "edge_right").position.x <= _rect(layout, "edge_left").end.x + 0.15,
		"An intervening inline edge suppresses automatic spacing")
	var resize: HTMLElement = layout.get_element_by_id("resize")
	var fr_wrap: HTMLElement = layout.get_element_by_id("fr_wrap")
	var cut: float = first.size.x + second.size.x + step / 2.0
	var word_width: float = _rect(layout, "fr_word").size.x
	_check(resize.set_attribute("style", "width: %spx" % cut) == OK and
		fr_wrap.set_attribute("style", "width: %spx" % word_width) == OK and
		layout.layout(Vector2i(480, 1200)) == OK, "Reflow a line that no longer fits the automatic gap")
	first = _rect(layout, "first")
	second = _rect(layout, "second")
	_check(second.position.y > first.position.y and absf(second.position.x - first.position.x) < 0.15,
		"Autospace contributes to wrapping and vanishes at the new line start")
	_check(is_equal_approx(_rect(layout, "fr_word").position.y, _rect(layout, "fr_mark").position.y),
		"French punctuation stays with its preceding word across a replaced space")
	_check(_has_diagnostic(layout, "text-autospace", "ideograph-alpha ideograph-alpha", "inline_style"),
		"Duplicate autospace keywords use declaration diagnostics")


func _check_punctuation_spacing() -> void:
	const FONT_UID: String = "{EAST_ASIAN_FONT_UID}"
	if FONT_UID.is_empty():
		return
	var font: FontFile = load(FONT_UID)
	_check(font != null, "Load font for CSS punctuation spacing")
	if font == null:
		return
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_local_fonts", {"res://fonts/east_asian.ttc": font,})
	document.text = """<!doctype html><style>
		@font-face { font-family: Eastern; src: url(fonts/east_asian.ttc); }
		body { margin: 0; font: 48px / 56px Eastern; }
		.spaced { text-spacing-trim: space-all; }
		#trim_start { text-spacing-trim: trim-start; }
		#trim_end { text-spacing-trim: trim-end; }
		#first_line { text-spacing-trim: space-first; width: 110px; }
		#tight, #allowed, #spaced_end { width: 75px; }
		#spaced_end { text-spacing-trim: space-start space-adjacent space-end; }
		#shorthand { --spacing: trim-all no-autospace; text-spacing: var(--spacing); }
		#reset { text-spacing: none; }
		#override { font-feature-settings: "halt" off; text-spacing-trim: trim-all; }
		#invalid { text-spacing-trim: trim-start space-start; }
		.transformed { display: inline-block; text-transform: full-width; }
		#transformed { text-spacing-trim: trim-all; }
		#intrinsic_grid { display: grid; grid-template-columns: min-content; width: 240px; }
		#support { width: 16px; height: 1px; }
		@supports (text-spacing: space-end trim-start no-autospace) and
			(not (text-spacing: trim-all space-end)) { #support { width: 49px; } }
	</style>
	<div class="spaced"><span id="full_open">「</span>水<span id="full_end">。</span></div>
	<div id="trim_start"><span id="start_mark">「</span>水</div>
	<div id="trim_end">水<span id="end_mark">。</span></div>
	<div><span id="first_open">「</span><span id="second_open">『</span>水</div>
	<div><span id="first_close">」</span><span id="second_close">』</span>水</div>
	<div><span id="space_close">」</span>　<span id="space_open">「</span>水</div>
	<div id="first_line"><span id="first_mark">「</span>永水<span id="soft_mark">「</span>山<br><span id="forced_mark">「</span>山</div>
	<div id="allowed">水<span id="allowed_mark">。</span></div>
	<div id="spaced_end">水<span id="spaced_end_mark">。</span></div>
	<div id="shorthand"><span id="always_open">「</span>A<span id="always_end">。</span>水</div>
	<div id="reset"><span id="reset_open">「</span><span id="reset_next">『</span>水</div>
	<div id="override"><span id="override_glyph">「</span>水</div>
	<div id="invalid"><span id="invalid_glyph">「</span>水</div>
		<div><span id="transformed" class="transformed">(A)</span><span id="transformed_ref" class="transformed spaced">(A)</span></div>
		<div id="intrinsic_grid"><div id="intrinsic">水。</div></div>
	<div id="support"></div>
	"""
	var layout: HTMLLayout = document.create_layout()
	layout.css_diagnostics_enabled = true
	_check(layout.layout(Vector2i(620, 900)) == OK, "Lay out CJK punctuation spacing")
	var full_open: float = _rect(layout, "full_open").size.x
	var full_end: float = _rect(layout, "full_end").size.x
	var trim_open: float = _rect(layout, "start_mark").size.x
	var trim_end: float = _rect(layout, "end_mark").size.x
	_check(trim_open < full_open - 10.0 and trim_end < full_end - 10.0,
		"Trimmed punctuation reduces the advance at either line edge: %s/%s, %s/%s" % [trim_open, full_open, trim_end, full_end])
	var expected: Dictionary[String, float] = {
		"first_open": full_open,
		"second_open": trim_open,
		"first_close": trim_end,
		"second_close": full_end,
		"space_close": trim_end,
		"space_open": trim_open,
		"first_mark": full_open,
		"soft_mark": trim_open,
		"forced_mark": full_open,
		"allowed_mark": trim_end,
		"spaced_end_mark": full_end,
		"always_open": trim_open,
		"always_end": trim_end,
		"reset_open": full_open,
		"reset_next": full_open,
		"override_glyph": full_open,
		"invalid_glyph": full_open,
		"support": 49.0,
	}
	for id: String in expected:
		var width: float = _rect(layout, id).size.x
		_check(absf(width - expected[id]) < 0.15, "Punctuation %s width %s vs %s" % [id, width, expected[id]])
	_check(_rect(layout, "soft_mark").position.y > _rect(layout, "first_mark").position.y,
		"Space-first trims opening punctuation on a soft-wrapped line")
	_check(_rect(layout, "transformed").size.x + 30.0 < _rect(layout, "transformed_ref").size.x,
		"Fullwidth punctuation produced by a text transform uses the same trimming")
	var intrinsic_width: float = _rect(layout, "intrinsic").size.x
	_check(absf(intrinsic_width - full_open - trim_end) < 0.15,
		"Grid min-content sizing uses the smallest fitting terminal punctuation advance: %s" % intrinsic_width)
	_check(_has_diagnostic(layout, "text-spacing-trim", "trim-start space-start", "style_element"),
		"Conflicting punctuation modes report a declaration diagnostic")
	var plain: String = layout.get_plain_text()
	var offset: int = plain.find("「A。水")
	var picked: Array[Rect2] = layout.get_selection_rects(offset, offset + 1)
	_check(offset >= 0 and picked.size() == 1 and absf(picked[0].size.x - trim_open) < 0.15,
		"Trimmed punctuation stays selectable under its original character")


func _check_text_spacing_paint(assets: Dictionary) -> void:
	var document: HTMLDocument = HTMLDocument.new()
	document.set("_local_fonts", assets)
	document.text = """<!doctype html><style>
		@font-face { font-family: Mono; src: url(fonts/cour.ttf); }
		body { margin: 0; font: 28px / 32px Mono; background: black; color: cyan; }
		#spaced { height: 65px; letter-spacing: 8.25px; text-shadow: 0 28px magenta; }
	</style><div id="spaced">MMMM</div><div>MMMM</div>"""
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(160, 110)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(viewport.size)
	label.document = document
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	var spaced: Rect2i = _color_bounds(image, Rect2i(0, 0, 150, 31), Color.CYAN)
	var base: Rect2i = _color_bounds(image, Rect2i(0, 65, 150, 40), Color.CYAN)
	var shadow: Rect2i = _color_bounds(image, Rect2i(0, 31, 150, 33), Color.MAGENTA)
	_check(spaced.has_area() and base.has_area() and absf(float(spaced.size.x - base.size.x) - 24.75) <= 2.0,
		"Painted glyphs use the fractional advances: %s vs %s" % [spaced, base])
	_check(shadow.has_area() and absi(spaced.size.x - shadow.size.x) <= 2,
		"Text-shadow uses the spaced glyph positions: %s vs %s" % [spaced, shadow])
	viewport.queue_free()


func _check_text_transform() -> void:
	var html: String = """<!doctype html><style>
		@font-face { font-family: MathProbe; src: url(fonts/cambria.ttc#Cambria%4Dath); }
		body { margin: 0; background: black; color: white; font: 24px / 32px MathProbe, sans-serif; }
		.row { position: relative; height: 32px; }
		.reference { position: absolute; top: 0; left: 240px; }
		.combined { --transform: full-size-kana full-width uppercase; text-transform: var(--transform); }
		.width { text-transform: full-width; }
		.kana { text-transform: full-size-kana; }
		.math { text-transform: math-auto; }
		.preserved { white-space: pre; }
		.invalid { text-transform: uppercase; text-transform: lowercase uppercase; text-transform: none full-width; }
		#duplicates { text-transform: var(--duplicate); --duplicate: full-width full-width; }
		#generated::before { content: 'α'; text-transform: math-auto; }
		#generated_ref::before { content: '𝛼'; }
	</style>
	<div class="row"><span id="combined" class="combined">aBｧㇰ𛅑</span><span class="reference">ＡＢアクゑ</span></div>
	<div class="row"><span class="kana">ぇｨㇿ𛅧</span><span class="reference">えｲロン</span></div>
	<div class="row"><span class="width">£ﾡ￩ｶﾞ</span><span class="reference">￡ㄱ←ガ</span></div>
	<div class="row"><span class="width preserved">A B</span><span class="reference preserved">Ａ　Ｂ</span></div>
	<div class="row"><span class="width">A  B</span><span class="reference">Ａ Ｂ</span></div>
	<div class="row"><span class="math">h</span><span class="reference">ℎ</span></div>
	<div class="row"><span id="math" class="math">ϴ</span><span class="reference">𝛳</span></div>
	<div class="row"><span id="math_multi" class="math">x y</span><span id="math_ref" class="reference">x y</span></div>
	<div class="row"><span class="math">∞</span><span class="reference">∞</span></div>
	<div class="row"><span id="generated"></span><span id="generated_ref" class="reference"></span></div>
	<div class="row"><span class="invalid">test</span><span class="reference">TEST</span></div>
	<div class="row"><span id="duplicates" class="width">ab</span><span class="reference">ab</span></div>"""
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	_check(layout.parse(html) == OK and layout.layout(Vector2i(480, 384)) == OK, "Lay out transformed text")
	var plain: String = layout.get_plain_text()
	_check(plain.begins_with("aBｧㇰ𛅑") and plain.count("ϴ") == 1 and plain.count("α") == 1,
		"Selection text preserves the original characters of styled and generated runs")
	var offset: int = plain.find("aBｧㇰ𛅑")
	var highlights: Array[Rect2] = layout.get_selection_rects(offset, offset + 5)
	var combined: Rect2 = _rect(layout, "combined")
	var bounds: Rect2
	for highlight: Rect2 in highlights:
		bounds = bounds.merge(highlight) if bounds.has_area() else highlight
	_check(bounds.has_area() and absf(bounds.size.x - combined.size.x) <= 0.1,
		"Selection covers the measured width across transformed text fragments")
	_check(layout.hit_test_text(Vector2(combined.end.x - 0.01, combined.get_center().y)) == offset + 5,
		"The transformed run maps hits back to source offsets")
	var math_offset: int = plain.find("ϴ")
	var math_box: Rect2 = _rect(layout, "math")
	_check(layout.get_selection_rects(math_offset, math_offset + 1).size() == 1 and
		layout.hit_test_text(Vector2(math_box.end.x - 0.01, math_box.get_center().y)) == math_offset + 1,
		"A supplementary mathematical glyph keeps one selectable source position")
	_check(_has_diagnostic(layout, "text-transform", "lowercase uppercase", "style_element") and
		_has_diagnostic(layout, "text-transform", "none full-width", "style_element") and
		_has_diagnostic(layout, "text-transform", "var(--duplicate)", "style_element"),
		"Conflicting, exclusive and duplicate values follow the declaration diagnostics")
	var math_multi: HTMLElement = layout.get_element_by_id("math_multi")
	var math_reference: HTMLElement = layout.get_element_by_id("math_ref")
	_check(math_multi.set_text_content("h") == OK and math_reference.set_text_content("ℎ") == OK and
		layout.layout(Vector2i(480, 384)) == OK and
		is_equal_approx(math_multi.get_bounding_rect().size.x, math_reference.get_bounding_rect().size.x),
		"A text node replaced through the DOM uses its new source length for math-auto")
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(480, 384)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(viewport.size)
	label.text = html
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	for row: int in range(12):
		var actual: Image = image.get_region(Rect2i(0, row * 32, 220, 32))
		var reference: Image = image.get_region(Rect2i(240, row * 32, 220, 32))
		_check(actual.get_data() == reference.get_data() and _font_ink(actual, Rect2(0, 0, 220, 32)) > 0.0,
			"Text transform row %d paints the reference glyphs" % row)
	label.select_all()
	_check(label.get_selected_text() == plain, "HTMLLabel copies the original text instead of the CSS-transformed glyphs")
	viewport.queue_free()


func _check_line_alignment() -> void:
	var html: String = """<!doctype html><style>
		body { margin: 0; background: black; color: white; font: 20px / 24px "Courier New", monospace; }
		div { width: 144px; }
		#wrapped { text-align: justify; text-align-last: center; text-justify: inter-word; }
		#justify_all { text-align: justify-all; text-justify: inter-word; }
		#none { text-align: justify-all; text-justify: none; }
		#reset { text-align-last: right; --alignment: justify; text-align: var(--alignment); }
		#parent_align { text-align-all: start; text-align-last: end; }
		#match { text-align: match-parent; }
		#full_width, #split { text-align: justify-all; }
		#full_width { text-justify: inter-character; }
		#split { text-justify: none; }
		@supports (text-justify: no-compress distribute) {
			#split { text-justify: var(--justify); --justify: no-compress distribute; }
		}
		#mixed { text-align: justify-all; text-justify: inter-character; }
		#exempt { text-justify: none; }
		#no_opportunity { text-align: justify; text-align-last: end; text-justify: inter-word; }
		#auto_cjk, #ruby, #cursive { text-align: justify-all; }
		#ruby { text-justify: ruby; }
		#cursive { text-justify: inter-character; }
		#resize { width: min(100%, 144px); text-align: justify-all; text-justify: inter-character; }
	</style>
	<div id="wrapped"><span id="wrapped_start">AA</span> <span id="wrapped_end">BB</span> <span id="before_break">CCCCCCCCC</span><br><span id="final">DD</span></div>
	<div id="justify_all"> <span id="words_start">AA</span> <span id="words_end">BB </span> </div>
	<div id="none"><span id="none_start">AA</span> <span id="none_end">BB</span></div>
	<div id="reset"><span id="reset_text">AA</span></div>
	<div id="parent_align"><div id="match"><span id="match_text">AA</span></div></div>
	<div id="full_width"><span id="characters">WXYZ</span></div>
	<div id="split"><span>WX</span><span id="split_end">YZ</span></div>
	<div id="mixed"><span id="exempt">WX</span><span id="mixed_end">YZ</span></div>
	<div id="no_opportunity"><span id="fallback">WWWWWWWW</span> <span>XXXXXXXX</span></div>
	<div id="auto_cjk">一二三</div>
	<div id="ruby"><span id="ruby_text">A B</span></div>
	<div id="cursive"><span id="cursive_text">سلام</span></div>
	<div id="resize"><span id="resize_text">TEXT</span></div>"""
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	_check(layout.parse(html) == OK and layout.layout(Vector2i(180, 420)) == OK, "Lay out default and last line alignment")
	var first: Rect2 = _rect(layout, "wrapped_start")
	var last: Rect2 = _rect(layout, "wrapped_end")
	_check(is_zero_approx(first.position.x) and absf(last.end.x - 144.0) <= 0.1 and absf(first.size.x - last.size.x) <= 0.1,
		"Inter-word justification expands only the gap on the first line")
	for id: String in ["before_break", "final", "ruby_text", "cursive_text"]:
		var box: Rect2 = _rect(layout, id)
		_check(box.has_area() and absf(box.get_center().x - 72.0) <= 0.1, "%s is centered on its own line" % id)
	var words: Rect2 = _rect(layout, "words_end")
	_check(is_zero_approx(_rect(layout, "words_start").position.x) and absf(words.end.x - 144.0) <= 0.1,
		"Justify-all fills the final line after trimming trailing spaces inside a span")
	var no_justify_start: Rect2 = _rect(layout, "none_start")
	var no_justify_end: Rect2 = _rect(layout, "none_end")
	_check(absf(no_justify_start.position.x + no_justify_end.end.x - 144.0) <= 0.1 and no_justify_start.position.x > 0.0,
		"Text-justify none follows the unexpandable-line alignment rule")
	_check(is_zero_approx(_rect(layout, "reset_text").position.x), "The deferred text-align shorthand resets text-align-last")
	_check(absf(_rect(layout, "match_text").end.x - 144.0) <= 0.1, "Match-parent inherits the final line's end alignment")
	_check(absf(_rect(layout, "fallback").end.x - 144.0) <= 0.1, "An unexpandable wrapped line uses text-align-last")
	var characters: Rect2 = _rect(layout, "characters")
	var split_end: Rect2 = _rect(layout, "split_end")
	_check(absf(characters.size.x - 144.0) <= 0.1 and absf(split_end.end.x - 144.0) <= 0.1,
		"Inter-character spacing expands both within a shaped run and across inline boundaries")
	_check(absf(_rect(layout, "exempt").size.x - first.size.x) <= 0.1 and absf(_rect(layout, "mixed_end").end.x - 144.0) <= 0.1,
		"A nested text-justify override keeps that run at its natural width")
	var plain: String = layout.get_plain_text()
	var char_offset: int = plain.find("WXYZ")
	var first_half: Array[Rect2] = layout.get_selection_rects(char_offset, char_offset + 2)
	_check(first_half.size() == 1 and absf(first_half[0].end.x - 72.0) <= 0.1 and
		layout.hit_test_text(Vector2(first_half[0].end.x, first_half[0].get_center().y)) == char_offset + 2,
		"Hit testing and selection use justified cluster advances")
	var cjk_offset: int = plain.find("一二三")
	var cjk: Array[Rect2] = layout.get_selection_rects(cjk_offset, cjk_offset + 3)
	_check(cjk.size() == 3 and absf(cjk[2].end.x - 144.0) <= 0.1, "Auto justification distributes space between Han glyphs")
	_check(not _has_diagnostic(layout, "text-justify", "var(--justify)", "style_element"), "Text-justify accepts variables and modifiers")
	var resized: HTMLElement = layout.get_element_by_id("resize_text")
	_check(is_equal_approx(resized.get_bounding_rect().size.x, 144.0) and
		layout.layout(Vector2i(101, 420)) == OK and is_equal_approx(resized.get_bounding_rect().size.x, 101.0) and
		layout.layout(Vector2i(180, 420)) == OK and is_equal_approx(resized.get_bounding_rect().size.x, 144.0),
		"Reflow computes fresh justification for the same text and element handle")

	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(180, 420)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(viewport.size)
	label.text = html
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	var whole_pixels: Image = image.get_region(Rect2i(0, roundi(_rect(layout, "full_width").position.y), 144, 24))
	var split_pixels: Image = image.get_region(Rect2i(0, roundi(_rect(layout, "split").position.y), 144, 24))
	_check(whole_pixels.get_data() == split_pixels.get_data() and _font_ink(whole_pixels, Rect2(0, 0, 144, 24)) > 0.0,
		"Painted inter-character spacing is invariant across plain inline tags")
	viewport.queue_free()


func _check_text_indent_and_tabs() -> void:
	var html: String = """<!doctype html><style>
		body { margin: 0; font: 20px/24px "Courier New", monospace; background: black; color: white; }
		.paragraph { width: 120px; }
		#each { --indent: each-line calc(10% + 3px); text-indent: var(--indent); }
		#hanging { text-indent: hanging 15px; }
		#both { text-indent: hanging each-line 15px; }
		#preline { white-space: pre-line; text-indent: 15px each-line; }
		#mixed_indent { text-indent: 15px; }
		#border_box { box-sizing: border-box; width: 140px; padding: 0 10px; text-indent: 10%; }
		#negative { margin-left: 20px; text-indent: -12px; }
		#intrinsic { display: inline-block; text-indent: calc(10% + 2px); }
		#duplicate { text-indent: 7px; text-indent: hanging 5px hanging; }
		#viewport_indent { width: min(100%, 160px); text-indent: 25%; }
		#float_indent { display: flow-root; text-indent: 15px each-line; }
		#late_float { float: right; width: 30px; height: 15px; }
		.tabs { white-space: pre; }
		.big { font-size: 40px; }
		#fraction { tab-size: calc(2 + .5); }
		#fixed { tab-size: 2em; }
		#fixed > span { font-size: 10px; }
		#tracked { tab-size: 4; letter-spacing: 2px; word-spacing: 3px; }
		#tight { tab-size: calc(1ch + 1px); }
		#zero { tab-size: 0; }
		#ordinary { tab-size: 0; }
		#bad_tab { tab-size: 24px; tab-size: -1; tab-size: 50%; }
		#wrap_tab { width: 62px; white-space: pre-wrap; tab-size: 30px; }
		#soft_tab, #forced_tab { width: 62px; white-space: pre-wrap; tab-size: 60px; text-align: right; }
		#float_tab { display: flow-root; tab-size: 24px; }
		#float_box { float: left; width: 32px; height: 24px; }
		#decorated { tab-size: 80px; }
		#tab_link { color: lime; text-decoration: underline; }
	</style>
	<div class="paragraph" id="each"><span id="each_start">AA</span> <span id="each_wrap">BBBBBBBB</span><br><span id="each_forced">CC</span></div>
	<div class="paragraph" id="hanging"><span id="hang_start">AA</span> <span id="hang_wrap">BBBBBBBB</span><br><span id="hang_forced">CC</span></div>
	<div class="paragraph" id="both"><span id="both_start">AA</span> <span id="both_wrap">BBBBBBBB</span><br><span id="both_forced">CC</span></div>
	<div id="preline">AA\n<span id="preline_forced">BB</span></div>
	<div id="mixed_indent"><span id="anon_start">AA</span><div>Block</div><span id="anon_after">BB</span></div>
	<div id="border_box"><span id="content_box">AA</span></div>
	<div id="negative"><span id="negative_text">AA</span></div>
	<div><span id="intrinsic">AAA</span></div>
	<div id="duplicate"><span id="duplicate_text">AA</span></div>
	<div id="viewport_indent"><span id="responsive_indent">AA</span></div>
	<div id="float_indent"><span id="before_float_break">AA</span><br><span id="after_break_and_float">BB</span><div id="late_float"></div></div>
	<div class="tabs"><span id="unit">A</span>\t<span id="default_tab">B</span>\t<span id="second_tab">C</span></div>
	<div class="tabs" id="fraction">AAA\t<span id="fraction_text">B</span></div>
	<div class="tabs" id="fixed"><span>A\t<span id="fixed_text">B</span></span></div>
	<div class="tabs" id="tracked">A<span class="big">\t</span><span id="tracked_text">B</span></div>
	<div class="tabs" id="tight">A\t<span id="tight_text">B</span></div>
	<div class="tabs" id="zero">ZERO\t<span id="zero_text">TAB</span></div>
	<div id="ordinary">A\t<span id="ordinary_text">B</span></div>
	<div class="tabs" id="bad_tab">A\t<span id="bad_text">B</span></div>
	<div id="wrap_tab">AAAA\t<span id="wrap_text">BBBB</span>\t<span id="wrapped_stop">C</span></div>
	<div id="soft_tab"><span id="soft_ending_tab">A</span>\tBBBB</div>
	<div id="forced_tab"><span id="forced_ending_tab">A</span>\t<br>BBBB</div>
	<div class="tabs" id="float_tab"><div id="float_box"></div>A\t<span id="float_text">B</span></div>
	<div class="tabs" id="decorated">A<a id="tab_link" href="#tab">\t</a>B</div>"""
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var viewport_size: Vector2i = Vector2i(230, 1024)
	_check(layout.parse(html) == OK and layout.layout(viewport_size) == OK, "Lay out indented lines and tab stops")
	var unit: float = _rect(layout, "unit").size.x
	var positions: Dictionary[String, float] = {
		"each_start": 15.0,
		"each_wrap": 0.0,
		"each_forced": 15.0,
		"hang_start": 0.0,
		"hang_wrap": 15.0,
		"hang_forced": 15.0,
		"both_start": 0.0,
		"both_wrap": 15.0,
		"both_forced": 0.0,
		"preline_forced": 15.0,
		"anon_start": 15.0,
		"anon_after": 0.0,
		"content_box": 22.0,
		"negative_text": 8.0,
		"duplicate_text": 7.0,
		"responsive_indent": 40.0,
		"after_break_and_float": 15.0,
		"default_tab": unit * 8.0,
		"second_tab": unit * 16.0,
		"fraction_text": unit * 5.0,
		"fixed_text": 40.0,
		"tracked_text": (unit + 5.0) * 4.0,
		"tight_text": (unit + 1.0) * 2.0,
		"zero_text": unit * 4.0,
		"ordinary_text": unit * 2.0,
		"bad_text": 24.0,
		"wrap_text": 0.0,
		"wrapped_stop": 0.0,
		"soft_ending_tab": 62.0 - unit,
		"forced_ending_tab": 2.0,
		"float_text": 72.0,
	}
	for id: String in positions:
		var actual: float = _rect(layout, id).position.x
		_check(is_equal_approx(actual, positions[id]), "%s starts at %s instead of %s" % [id, actual, positions[id]])
	_check(absf(_rect(layout, "intrinsic").size.x - unit * 3.0 - 2.0) <= 0.1,
		"Intrinsic width includes the fixed part of the indent and gives percentages a zero basis")
	_check(_rect(layout, "each_wrap").position.y > _rect(layout, "each_start").position.y and
		_rect(layout, "wrapped_stop").position.y > _rect(layout, "wrap_text").position.y,
		"Soft wrapping uses the line's indent and recomputes tabs from the new line position")
	_check(_rect(layout, "after_break_and_float").position.y > _rect(layout, "before_float_break").position.y,
		"A late float keeps the preceding forced break when rebuilding the current line")
	var responsive_indent: HTMLElement = layout.get_element_by_id("responsive_indent")
	_check(layout.layout(Vector2i(101, viewport_size.y)) == OK and
		is_equal_approx(responsive_indent.get_bounding_rect().position.x, 25.25) and
		layout.layout(viewport_size) == OK and is_equal_approx(responsive_indent.get_bounding_rect().position.x, 40.0),
		"Percentage indentation follows the current content width across reflows")
	_check(_has_diagnostic(layout, "text-indent", "hanging 5px hanging", "style_element") and
		_has_diagnostic(layout, "tab-size", "-1", "style_element") and
		_has_diagnostic(layout, "tab-size", "50%", "style_element"),
		"Invalid indent modifiers and tab sizes preserve preceding declarations")
	var plain: String = layout.get_plain_text()
	_check("ZERO\tTAB" in plain and "A B" in plain and plain.ends_with("A\tB"),
		"Copied preserved and zero-width tabs remain single characters")
	var tab_offset: int = plain.rfind("\t")
	var tab_box: Rect2 = _rect(layout, "tab_link")
	var selection: Array[Rect2] = layout.get_selection_rects(tab_offset, tab_offset + 1)
	_check(selection.size() == 1 and absf(selection[0].size.x - tab_box.size.x) <= 0.1 and
		layout.hit_test_text(Vector2(tab_box.end.x - 1.0, tab_box.get_center().y)) == tab_offset + 1,
		"A preserved tab has one selectable interval across its full advance")
	_check(layout.get_link_at(tab_box.get_center()) == "#tab", "Tab advances participate in inline link hit testing")

	var viewport: SubViewport = SubViewport.new()
	viewport.size = viewport_size
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(viewport.size)
	label.text = html
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	var y: int = roundi(_rect(layout, "decorated").position.y)
	var underline_area: Rect2i = Rect2i(0, y, 95, 24).intersection(Rect2i(Vector2i.ZERO, image.get_size()))
	var underline: Rect2i = _color_bounds(image, underline_area, Color.LIME)
	_check(underline.has_area() and absi(underline.position.x - roundi(unit)) <= 1 and absi(underline.end.x - 80) <= 1,
		"The underline paints the gap occupied by a preserved tab")
	label.select_all()
	_check(label.get_selected_text() == plain, "HTMLLabel copies the original tab characters")
	viewport.queue_free()


func _check_white_space() -> void:
	var html: String = """<!doctype html><style>
		body { margin: 0; font: 20px/24px "Courier New", monospace; background: black; color: white; }
		#spaces { width: 3ch; white-space: break-spaces; }
		#spaces a { text-decoration: none; background: lime; }
		#lookback { width: 4ch; white-space: wrap break-spaces; }
		#no_prior { width: 2ch; white-space: break-spaces; }
		#tabs { width: 42px; white-space: break-spaces; tab-size: 30px; }
		#preserve { --space: preserve-spaces nowrap; white-space: var(--space); }
		#preserve_break { white-space: nowrap; white-space-collapse: preserve-breaks; }
		#discard { white-space-collapse: discard; }
		#local_nowrap { width: 6ch; }
		#local_group { text-wrap-mode: nowrap; }
		#local_wrap { width: 4ch; text-wrap-mode: nowrap; }
		#local_wrap > span { white-space: collapse wrap; }
		#inherited { white-space: pre; width: 3ch; }
		#inherited > div { text-wrap-mode: wrap; }
		#shorthand { white-space-collapse: preserve; text-wrap-mode: nowrap; }
		#shorthand > div { white-space: normal; }
		#unicode_spaces { width: 25px; white-space-collapse: break-spaces; }
		#forced { white-space: nowrap; }
		#bad { white-space: wrap preserve; white-space: preserve nowrap wrap; text-wrap-mode: balance; }
		.width_probe { display: grid; width: 1px; grid-template-columns: 1fr; }
		#break_minimum { white-space: break-spaces; }
		#nowrap_minimum span { text-wrap-mode: nowrap; }
	</style>
	<div id="spaces"><span id="unit_ws">A</span><a href="#space" id="space_one"> </a><a href="#space" id="space_two"> </a><a href="#space" id="space_three"> </a><a href="#space" id="space_four"> </a><span id="after_spaces">B</span></div>
	<div id="lookback"><span id="before_group">AA</span> <span id="word_group">B</span><span id="held_space"> </span>C</div>
	<div id="no_prior"><span id="before_required">AA</span><span id="required"> </span></div>
	<div id="tabs"><span id="first_tab_word">A</span><span id="first_ws_tab">\t</span><span id="second_ws_tab">\t</span><span id="after_tabs">B</span></div>
	<div id="preserve">START\t\n  <span id="preserve_end">END</span></div>
	<div id="preserve_break">BREAK  \t \n  <span id="preserved_next">NEXT</span></div>
	<div id="discard">REMOVE \t\n <span id="after_discard">SPACES</span></div>
	<div id="local_nowrap"><span id="outside_nowrap">AA</span> <span id="local_group"><span id="first_group_word">BB</span> <span id="last_group_word">CC</span></span></div>
	<div id="local_wrap"><span><span id="first_local_word">AA</span> <span id="next_local_word">BB</span></span></div>
	<div id="inherited"><div>AA  <span id="inherited_end">BB</span></div></div>
	<div id="shorthand"><div>AA  <span id="shorthand_end">BB</span></div></div>
	<div id="unicode_spaces"><span id="before_en">A</span><span id="first_en">&ensp;</span><span id="second_en">&ensp;</span>B</div>
	<div id="forced"><span id="forced_before">AA</span>&#x2028;<span id="forced_after">BB</span></div>
	<div id="bad">A  <span id="bad_ws_end">B</span></div>
	<div class="width_probe"><div id="break_minimum">WW  Q</div></div>
	<div class="width_probe"><div id="nowrap_minimum"><span>WX YZ</span> Q</div></div>"""
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	_check(layout.parse(html) == OK and layout.layout(Vector2i(220, 600)) == OK, "Lay out CSS whitespace modes")
	var unit: float = _rect(layout, "unit_ws").size.x
	var first_line: float = _rect(layout, "space_one").position.y
	var second_line: float = _rect(layout, "space_three").position.y
	_check(second_line > first_line and is_equal_approx(_rect(layout, "space_two").position.y, first_line) and
		is_equal_approx(_rect(layout, "space_four").position.y, second_line) and
		is_equal_approx(_rect(layout, "after_spaces").position.x, unit * 2.0),
		"Break-spaces wraps between consecutive spaces while preserving their advances")
	_check(_rect(layout, "word_group").position.y > _rect(layout, "before_group").position.y and
		is_equal_approx(_rect(layout, "held_space").position.y, _rect(layout, "word_group").position.y) and
		is_equal_approx(_rect(layout, "word_group").position.x, 0.0),
		"A first preserved space stays with its word by using the preceding wrap opportunity")
	_check(is_equal_approx(_rect(layout, "required").position.y, _rect(layout, "before_required").position.y) and
		is_equal_approx(_rect(layout, "required").end.x, unit * 3.0),
		"The first space can overflow with its word when no prior wrap opportunity exists")
	_check(_rect(layout, "second_ws_tab").position.y > _rect(layout, "first_ws_tab").position.y and
		is_equal_approx(_rect(layout, "second_ws_tab").size.x, 30.0) and
		is_equal_approx(_rect(layout, "after_tabs").position.x, 30.0),
		"A preserved tab wraps after the previous tab and recalculates its advance on the new line")
	_check(is_equal_approx(_rect(layout, "preserve_end").position.x, unit * 9.0) and
		is_equal_approx(_rect(layout, "preserved_next").position.x, 0.0),
		"Preserve-spaces converts tabs and newlines while preserve-breaks retains the newline")
	_check(is_equal_approx(_rect(layout, "after_discard").position.x, unit * 6.0) and
		layout.get_element_by_id("discard").get_text_content() == "REMOVE \t\n SPACES",
		"Discard removes CSS whitespace during layout and retains DOM text")
	_check(_rect(layout, "first_group_word").position.y > _rect(layout, "outside_nowrap").position.y and
		is_equal_approx(_rect(layout, "first_group_word").position.y, _rect(layout, "last_group_word").position.y) and
		is_equal_approx(_rect(layout, "first_group_word").position.x, 0.0),
		"An inherited nowrap span moves to a previous allowed break and stays on one line")
	_check(_rect(layout, "next_local_word").position.y > _rect(layout, "first_local_word").position.y and
		is_equal_approx(_rect(layout, "inherited_end").position.x, 0.0) and
		is_equal_approx(_rect(layout, "shorthand_end").position.x, unit * 3.0),
		"A local wrap mode and the white-space shorthand override inherited wrapping and collapsing independently")
	_check(_rect(layout, "second_en").position.y > _rect(layout, "first_en").position.y and
		_rect(layout, "forced_after").position.y > _rect(layout, "forced_before").position.y,
		"Unicode separators can wrap in break-spaces and mandatory Unicode breaks override nowrap")
	_check(is_equal_approx(_rect(layout, "bad_ws_end").position.x, unit * 3.0) and
		_has_diagnostic(layout, "white-space", "preserve nowrap wrap", "style_element") and
		_has_diagnostic(layout, "text-wrap-mode", "balance", "style_element"),
		"Invalid whitespace values retain the previous valid declaration")
	_check(is_equal_approx(_rect(layout, "break_minimum").size.x, unit * 3.0) and
		is_equal_approx(_rect(layout, "nowrap_minimum").size.x, unit * 5.0),
		"Grid min-content sizing includes the word's first preserved space and all text in a nowrap span")
	var plain: String = layout.get_plain_text()
	_check(plain.begins_with("A    B") and "START    END" in plain and "BREAK\nNEXT" in plain and "REMOVESPACES" in plain,
		"Copy keeps preserved spacing and omits discarded whitespace without inserting newlines at soft wraps")
	var second_space: Rect2 = _rect(layout, "space_two")
	var selected_space: Array[Rect2] = layout.get_selection_rects(2, 3)
	_check(selected_space.size() == 1 and absf(selected_space[0].size.x - unit) <= 0.1 and
		layout.get_link_at(second_space.get_center()) == "#space", "Trailing preserved spaces are selectable and clickable")

	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(220, 300)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(viewport.size)
	label.text = html
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	_check_pixel(image, second_space.get_center(), Color.LIME, "A trailing break-spaces box paints at its measured position")
	_check_pixel(image, _rect(layout, "space_three").get_center(), Color.LIME, "A wrapped space paints at its new line position")
	viewport.queue_free()


func _check_white_space_trim() -> void:
	var html: String = """<!doctype html><style>
		body { margin: 0; font: 20px/24px "Courier New", monospace; background: black; color: white; }
		.pre { white-space: pre; }
		.trim_inner { white-space-trim: discard-inner; }
		#outer_trim { white-space-trim: discard-after discard-before; }
		#outer_trim, #inline_trim { background: lime; color: black; text-decoration: none; }
		#inline_trim { --trim: preserve discard-inner nowrap; white-space: var(--trim); }
		#reset { white-space-trim: discard-inner; white-space: pre; }
		#explicit_break { white-space: preserve discard-inner; }
		#joined { width: 2ch; }
		#joined_end { white-space-trim: discard-before; }
		#preserved_edges { white-space-trim: discard-before discard-after; }
		#invalid { white-space-trim: discard-inner; white-space-trim: none discard-before; }
		#invalid { white-space: preserve discard-inner discard-inner; }
		#generated_term::before { content: ', '; white-space-trim: discard-before; }
		#outside_breaks { white-space: pre-line; }
		#after_source_break { white-space-trim: discard-before; }
		@media (max-width: 180px) { #media { white-space-trim: discard-inner; } }
	</style>
	<div id="external">LEFT<span> \t</span><a href="#trim" id="outer_trim">M</a><span><i> </i></span><span id="external_end">RIGHT</span></div>
	<div id="nested">A<a href="#nested" id="inline_trim"><b>  \n<i id="trimmed_word">X</i>\t</b>  </a>B</div>
	<div id="block_trim" class="pre trim_inner">\t\n \n  <span id="block_start">P</span> \n  <span id="block_end">Q</span>  \n\t \n</div>
	<div id="single_line" class="pre trim_inner">  <span id="single_word">W</span>  </div>
	<div id="non_inherited" class="pre trim_inner">A<span>  <b id="ordinary_child">Y</b>  </span>B</div>
	<div>A<span id="reset">  <b id="reset_word">Z</b>  </span>B</div>
	<div id="explicit_before">START<span id="explicit_break"> \n<br><b id="after_explicit">LINE</b>\n \t</span></div>
	<div id="joined"><span id="joined_start">AA</span> <span id="joined_end">BB</span></div>
	<div id="preserved_line" class="pre">A <span id="preserved_edges">B</span> <b id="preserved_end">C</b></div>
	<div class="pre">A<span id="invalid">  <b id="invalid_word">V</b>  </span>B</div>
	<div class="pre">A<span id="media">  <b id="media_word">T</b>  </span>B</div>
	<div>FIRST <span id="generated_term">SECOND</span></div>
	<div id="outside_breaks">PROTECTED \n<span id="after_source_break">NEXT</span></div>
	<div class="pre trim_inner"> \n</div>"""
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var viewport_size: Vector2i = Vector2i(235, 500)
	_check(layout.parse(html) == OK and layout.layout(viewport_size) == OK, "Lay out the whitespace trimming modes")
	var unit: float = _rect(layout, "trimmed_word").size.x
	var external: Rect2 = _rect(layout, "outer_trim")
	var inner: Rect2 = _rect(layout, "inline_trim")
	_check(external.has_area() and is_equal_approx(external.position.x, unit * 4.0) and
		is_equal_approx(_rect(layout, "external_end").position.x, unit * 5.0),
		"Both exterior edges trim collapsible spaces across nested inline boundaries")
	_check(inner.has_area() and is_equal_approx(inner.position.x, unit) and is_equal_approx(inner.size.x, unit),
		"Inline discard-inner trims nested preserved spaces, tabs and source line breaks")
	var block: Rect2 = _rect(layout, "block_trim")
	var first: Rect2 = _rect(layout, "block_start")
	var last: Rect2 = _rect(layout, "block_end")
	_check(is_equal_approx(first.position.x, unit * 2.0) and is_equal_approx(last.position.x, first.position.x) and
		is_equal_approx(last.position.y - first.position.y, 24.0) and is_equal_approx(block.size.y, 48.0),
		"Block trimming removes whole boundary blank lines and preserves the first line's indentation")
	_check(is_equal_approx(_rect(layout, "single_word").position.x, unit * 2.0),
		"A single-line preformatted block keeps spaces when no source line break is present")
	_check(is_equal_approx(_rect(layout, "ordinary_child").position.x, unit * 3.0) and
		is_equal_approx(_rect(layout, "reset_word").position.x, unit * 3.0),
		"Trim is non-inherited and an omitted shorthand value resets it")
	_check(_rect(layout, "after_explicit").position.y > _rect(layout, "explicit_before").position.y and
		is_equal_approx(_rect(layout, "preserved_end").position.x, unit * 4.0),
		"Exterior trimming retains preserved spaces and interior trimming retains explicit break elements")
	_check(is_equal_approx(_rect(layout, "joined_end").position.y, _rect(layout, "joined_start").position.y) and
		is_equal_approx(_rect(layout, "joined_end").position.x, unit * 2.0),
		"Trimming the only space inside an ASCII word removes its soft wrap opportunity")
	_check(is_equal_approx(_rect(layout, "invalid_word").position.x, unit) and
		_has_diagnostic(layout, "white-space-trim", "none discard-before", "style_element") and
		_has_diagnostic(layout, "white-space", "preserve discard-inner discard-inner", "style_element"),
		"Invalid trim combinations preserve valid declarations")
	var plain: String = layout.get_plain_text()
	_check(plain.begins_with("LEFTMRIGHT\nAXB\n  P \n  Q  ") and
		"START\nLINE" in plain and "PROTECTED\nNEXT" in plain,
		"Copied text includes the displayed spaces and just the surviving breaks")
	_check("FIRST, SECOND" in plain, "Trim on a generated inline box reaches adjacent collapsible whitespace")
	var trimmed_element: HTMLElement = layout.get_element_by_id("inline_trim")
	_check(trimmed_element.get_text_content() == "  \nX\t  ", "Visual trimming leaves original DOM text intact")
	var selected: Array[Rect2] = layout.get_selection_rects(4, 5)
	_check(selected.size() == 1 and is_equal_approx(selected[0].position.x, external.position.x) and
		layout.get_link_at(external.get_center()) == "#trim", "Selection and link geometry follow the trimmed positions")
	_check(layout.layout(Vector2i(170, viewport_size.y)) == OK and
		is_equal_approx(_rect(layout, "media_word").position.x, unit) and
		layout.layout(viewport_size) == OK and is_equal_approx(_rect(layout, "media_word").position.x, unit * 3.0),
		"Media restyles trim and restore whitespace without replacing the DOM")
	_check(trimmed_element.set_attribute("style", "white-space-trim:none") == OK and
		layout.layout(viewport_size) == OK and trimmed_element.is_valid() and "  \nX\t  " in layout.get_plain_text(),
		"DOM style mutations restore the original whitespace runs")

	var viewport: SubViewport = SubViewport.new()
	viewport.size = viewport_size
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(viewport_size)
	label.text = html
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	_check_pixel(image, external.position + Vector2(1.0, 1.0), Color.LIME, "Trimmed exterior spacing matches the painted box")
	_check_pixel(image, inner.position + Vector2(1.0, 1.0), Color.LIME, "Trimmed interior spacing matches the painted box")
	label.select_all()
	_check(label.get_selected_text() == plain, "HTMLLabel copies the trimmed layout text")
	viewport.queue_free()


func _check_text_wrap_styles() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var styles: Dictionary[String, String] = {
		"auto": "text-wrap-style: auto",
		"stable": "text-wrap: stable",
		"balance": "text-wrap: wrap balance",
		"pretty": "--strategy: pretty wrap; text-wrap: var(--strategy)",
		"tail": "text-wrap-style: avoid-short-last-line",
		"nowrap": "text-wrap: balance nowrap",
		"reset_mode": "text-wrap-mode: nowrap; text-wrap: balance",
		"reset_style": "text-wrap-style: balance; text-wrap: wrap",
		"invalid": "text-wrap: balance; text-wrap: pretty balance",
	}
	var source: String = """<!doctype html><style>
		body { margin: 0; font: 20px monospace; line-height: 24px; }
		div { width: 11.4ch; }
		#hard { text-wrap: balance; text-align: right; }
		#clamped_tail { text-wrap-style: avoid-short-last-line; }
		@supports (text-wrap: pretty) { #supported { text-wrap-style: balance; } }
	</style>"""
	for id: String in styles:
		source += (
			'<div id="%s" style="%s"><span id="%s-a">AA</span> BB CC ' +
			'<a id="%s-d" href="#hard">DD</a> <span id="%s-e">EE</span></div>'
		) % [id, styles[id], id, id, id]
	source += """<div id="hard"><span id="hard-a">AA</span> BB CC <span id="hard-d">DD</span> <span id="hard-e">EE</span><br>
		<span id="hard-f">FF</span> GG HH <span id="hard-i">II</span> <span id="hard-j">JJ</span></div>
		<div id="clamped_tail"><span id="tail_a">XX</span> XX XX <span id="tail_d">XX</span>
			<span id="tail_e">XX</span> XX XX <span id="tail_h">XX</span> <span id="tail_i">XX</span></div>
		<div id="supported">AA BB CC <span id="supported-d">DD</span> <span id="supported-e">EE</span></div>"""
	if layout.parse(source) != OK or layout.layout(Vector2i(260, 800)) != OK:
		_check(false, "Prepare text-wrap fixture: " + layout.get_render_error())
		return
	for id: String in ["auto", "stable", "reset_style"]:
		_check(is_equal_approx(_rect(layout, id + "-a").position.y, _rect(layout, id + "-d").position.y) and
			_rect(layout, id + "-e").position.y > _rect(layout, id + "-d").position.y,
			"%s uses the ordinary first-fit break" % id)
	for id: String in ["balance", "pretty", "tail", "reset_mode", "invalid"]:
		var link: Rect2 = _rect(layout, id + "-d")
		_check(link.position.y > _rect(layout, id + "-a").position.y and
			is_equal_approx(link.position.y, _rect(layout, id + "-e").position.y),
			"%s moves the fourth word to the last line" % id)
		_check(layout.get_link_at(link.get_center()) == "#hard", "%s moves the link hit region" % id)
		_check(is_equal_approx(_rect(layout, id).size.y, _rect(layout, "auto").size.y) and
			is_equal_approx(_rect(layout, id).size.x, _rect(layout, "auto").size.x),
			"%s preserves the line count and box width" % id)
	_check(is_equal_approx(_rect(layout, "nowrap-a").position.y, _rect(layout, "nowrap-e").position.y),
		"An explicit nowrap mode disables soft breaks")
	_check(_has_diagnostic(layout, "text-wrap", "pretty balance", "inline_style"),
		"A duplicate text-wrap style is invalid")
	_check(is_equal_approx(_rect(layout, "supported-d").position.y, _rect(layout, "supported-e").position.y),
		"The text-wrap shorthand participates in supports conditions")
	_check(_rect(layout, "hard-d").position.y > _rect(layout, "hard-a").position.y and
		is_equal_approx(_rect(layout, "hard-d").position.y, _rect(layout, "hard-e").position.y) and
		_rect(layout, "hard-i").position.y > _rect(layout, "hard-f").position.y and
		is_equal_approx(_rect(layout, "hard-i").position.y, _rect(layout, "hard-j").position.y),
		"Forced breaks separate independently balanced line groups")
	_check(is_equal_approx(_rect(layout, "hard-j").end.x, _rect(layout, "hard").end.x),
		"Balanced text still aligns to the full box width")
	_check(is_equal_approx(_rect(layout, "tail_a").position.y, _rect(layout, "tail_d").position.y) and
		_rect(layout, "tail_e").position.y > _rect(layout, "tail_d").position.y and
		is_equal_approx(_rect(layout, "tail_h").position.y, _rect(layout, "tail_i").position.y),
		"The last-line strategy leaves the earlier line alone")
	var balance: HTMLElement = layout.get_element_by_id("balance")
	_check(balance.set_attribute("style", "text-wrap: balance; width: 16ch") == OK and
		layout.layout(Vector2i(260, 800)) == OK, "Reflow balanced text after a width change")
	_check(is_equal_approx(_rect(layout, "balance-a").position.y, _rect(layout, "balance-e").position.y),
		"Balancing does not introduce unnecessary breaks in a single line")


func _check_cjk_word_break() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var source: String = """<!doctype html><style>
		body { margin: 0; font-size: 20px; line-height: 26px; }
		p { margin: 0; width: 2.45em; }
		.keep { word-break: keep-all; }
		#nowrap { text-wrap-mode: nowrap; }
		#hidden wbr { display: none; }
		@supports (word-break: keep-all) { #supported { word-break: keep-all; } }
		.grid { display: grid; grid-template-columns: auto minmax(0, 1fr); width: 20px; }
	</style>
	<p id="normal">日本語字</p>
	<p id="keep" class="keep">日本語字</p>
	<p id="supported">日本語字</p>
	<p id="korean">한글문자</p>
	<p id="korean_keep" class="keep">한글문자</p>
	<p id="kana">カタカナ</p>
	<p id="kana_keep" class="keep">カタカナ</p>
	<p id="punct_open">日<span id="open_mark">「</span><span id="open_next">本</span>語</p>
	<p id="punct_close">日<span id="close_prev">本</span><span id="close_mark">、</span>語</p>
	<p id="explicit" class="keep"><span id="wbr_first">日本</span><wbr><a id="wbr_last" href="#normal">語字</a></p>
	<p id="zwsp" class="keep"><span id="zwsp_first">日本</span>&#x200B;<span id="zwsp_last">語字</span></p>
	<p id="hidden" class="keep">日本<wbr>語字</p>
	<p id="nowrap" class="keep">日本<wbr>語字</p>
	<p id="leading" class="keep"><wbr>日本語字</p>
	<p id="only_wbr"><wbr></p>
	<p id="latin" class="keep" style="font-family: monospace; width: 2.5ch"><span>AB</span><wbr><span>CD</span></p>
	<p id="invalid" class="keep" style="word-break: bogus">日本語字</p>
	<div class="grid"><span id="normal_track">日本語字</span><i></i></div>
	<div class="grid keep"><span id="keep_track">日本語字</span><i></i></div>
	"""
	if layout.parse(source) != OK or layout.layout(Vector2i(260, 800)) != OK:
		_check(false, "Prepare CJK word-break fixture: " + layout.get_render_error())
		return
	var checks: Dictionary[String, String] = {
		"normal": "keep",
		"korean": "korean_keep",
		"kana": "kana_keep",
	}
	for id: String in checks:
		var wrapped: Rect2 = _rect(layout, id)
		var unwrapped: Rect2 = _rect(layout, checks[id])
		_check(wrapped.size.y > unwrapped.size.y, "%s wraps normally and inherits keep-all" % id)
	_check(is_equal_approx(_rect(layout, "supported").size.y, _rect(layout, "keep").size.y),
		"Keep-all participates in supports conditions")
	_check(is_equal_approx(_rect(layout, "open_mark").position.y, _rect(layout, "open_next").position.y),
		"An opening bracket stays with the following character")
	_check(is_equal_approx(_rect(layout, "close_prev").position.y, _rect(layout, "close_mark").position.y),
		"A closing punctuation mark stays with the preceding character")
	var link: Rect2 = _rect(layout, "wbr_last")
	_check(link.position.y > _rect(layout, "wbr_first").position.y and layout.get_link_at(link.get_center()) == "#normal",
		"A wbr remains breakable with keep-all, and the moved link is hittable")
	_check(_rect(layout, "zwsp_last").position.y > _rect(layout, "zwsp_first").position.y,
		"A zero-width space remains breakable with keep-all")
	_check(_rect(layout, "latin").size.y > _rect(layout, "keep").size.y,
		"A wbr also introduces an opportunity within a Latin word")
	_check("日本\u200b語字" in layout.get_plain_text() and layout.get_element_by_id("zwsp").get_text_content() == "日本\u200b語字",
		"A zero-width space survives copying without a synthetic line feed")
	for id: String in ["hidden", "nowrap", "leading", "invalid"]:
		_check(is_equal_approx(_rect(layout, id).size.y, _rect(layout, "keep").size.y),
			"%s preserves the unbroken run" % id)
	_check(is_zero_approx(_rect(layout, "only_wbr").size.y), "A lone wbr does not create a line")
	_check(_has_diagnostic(layout, "word-break", "bogus", "inline_style"), "Reject invalid word-break values")
	_check(_rect(layout, "keep_track").size.x > _rect(layout, "normal_track").size.x,
		"Keep-all uses the unbroken CJK run for the grid's minimum width")
	var element: HTMLElement = layout.get_element_by_id("keep")
	_check(element.set_attribute("style", "word-break: var(--missing, normal)") == OK and
		layout.layout(Vector2i(260, 800)) == OK, "Update word-break through the DOM")
	_check(is_equal_approx(element.get_bounding_rect().size.y, _rect(layout, "normal").size.y),
		"Normal restores the CJK break opportunities after a DOM update")


func _check_line_break_strictness() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var source: String = """<!doctype html><style>
		body { margin: 0; font-size: 20px; line-height: 26px; }
		p { margin: 0; width: 360px; }
		article { line-break: loose; }
		#responsive { line-break: loose; }
		#invalid { line-break: strict; line-break: bogus; }
		#feature { line-break: strict; }
		@supports (line-break: loose) { #feature { line-break: loose; } }
	</style><article lang="ja-JP">"""
	var cases: Array[Dictionary] = [
		{"id": "kana_loose", "left": "本", "right": "ぁ", "split": true},
		{"id": "kana_normal", "left": "本", "right": "ぁ", "split": false, "style": "line-break: normal"},
		{"id": "kana_keep", "left": "本", "right": "ぁ", "split": true, "style": "word-break: keep-all"},
		{"id": "iteration", "left": "本", "right": "々", "split": true},
		{"id": "ellipse_loose", "left": "…", "right": "…", "split": true},
		{"id": "ellipse_auto", "left": "…", "right": "…", "split": false, "style": "line-break: auto"},
		{"id": "wave_normal", "left": "本", "right": "〜", "split": true, "style": "line-break: normal"},
		{"id": "wave_strict", "left": "本", "right": "〜", "split": false, "style": "--mode: strict; line-break: var(--mode)"},
		{"id": "wave_latin", "left": "本", "right": "〜", "split": false, "lang": "ja-Latn"},
		{"id": "wave_hanja", "left": "本", "right": "〜", "split": true, "lang": "ko-Hani"},
		{"id": "wave_untagged", "left": "本", "right": "〜", "split": false, "lang": ""},
		{"id": "dash", "left": "本", "right": "‐", "split": true},
		{"id": "dash_latin", "left": "B", "right": "‐", "split": false},
		{"id": "feature", "left": "本", "right": "・", "split": true, "lang": "zh-Hant", "style": ""},
		{"id": "center_strict", "left": "本", "right": "・", "split": false, "style": "line-break: strict"},
		{"id": "suffix", "left": "８", "right": "％", "split": true},
		{"id": "suffix_unknown", "left": "８", "right": "％", "split": false, "lang": "en"},
		{"id": "prefix", "left": "￥", "right": "８", "split": true},
		{"id": "prefix_normal", "left": "￥", "right": "８", "split": false, "style": "line-break: normal"},
		{"id": "opening", "left": "「", "right": "本", "split": false},
		{"id": "closing", "left": "本", "right": "、", "split": false},
		{"id": "invalid", "left": "本", "right": "〜", "split": false},
		{"id": "responsive", "left": "本", "right": "・", "split": true},
	]
	for case: Dictionary in cases:
		var id: String = case.id
		var language: String = ' lang="%s"' % case.lang if case.has("lang") else ""
		var leading: String = "A" if id == "dash_latin" else "日"
		source += ('<p id="%s"%s style="%s"><span id="%s-first">%s</span>' +
			'<span id="%s-left">%s</span><span id="%s-right">%s</span>語</p>') % [
			id, language, case.get("style", ""), id, leading, id, case.left, id, case.right,
		]
	source += "</article><p id='direct' lang='ja' style='line-break: loose'>日本・語</p>"
	if layout.parse(source) != OK or layout.layout(Vector2i(400, 1600)) != OK:
		_check(false, "Prepare line-break fixture: " + layout.get_render_error())
		return
	for case: Dictionary in cases:
		var id: String = case.id
		var width: float = _rect(layout, id + "-left").end.x - _rect(layout, id + "-first").position.x
		var row: HTMLElement = layout.get_element_by_id(id)
		var style: String = "%s; width: %.3fpx" % [case.get("style", ""), width + 0.2]
		_check(row.set_attribute("style", style) == OK, "Set the exact line width for " + id)
	var direct: HTMLElement = layout.get_element_by_id("direct")
	var width: float = _rect(layout, "kana_loose-left").end.x - _rect(layout, "kana_loose-first").position.x
	_check(direct.set_attribute("style", "line-break: loose; width: %.3fpx" % (width + 0.2)) == OK,
		"Resize adjacent text in a single source node")
	_check(layout.layout(Vector2i(400, 1600)) == OK, "Lay out all line-break modes at a narrow width")
	for case: Dictionary in cases:
		var id: String = case.id
		var left: Rect2 = _rect(layout, id + "-left")
		var right: Rect2 = _rect(layout, id + "-right")
		var split: bool = right.position.y > left.position.y
		_check(split == case.split, "line-break %s has split=%s, expected %s" % [id, split, case.split])
	_check("日本・語" in layout.get_plain_text() and direct.get_bounding_rect().size.y > 26.0,
		"Line breaking splits source runs at punctuation without inserting copied line feeds")
	_check(_has_diagnostic(layout, "line-break", "bogus", "style_element"),
		"An invalid later line-break declaration leaves strict mode in effect")
	var responsive: HTMLElement = layout.get_element_by_id("responsive")
	_check(responsive.set_attribute("lang", "ja-Latn") == OK and layout.layout(Vector2i(400, 1600)) == OK,
		"Reflow after updating the writing system")
	_check(is_equal_approx(_rect(layout, "responsive-left").position.y, _rect(layout, "responsive-right").position.y),
		"Changing lang recomputes the break opportunities")


func _check_line_break_anywhere() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	var source: String = """<!doctype html><style>
		body { margin: 0; font: 20px/24px monospace; }
		.probe { width: 5.5ch; }
		.anywhere { line-break: anywhere; }
		#nowrap { white-space: nowrap; }
		#clusters { width: 1.5ch; }
		#spaces { width: 3.5ch; white-space: break-spaces; }
		.grid { display: grid; grid-template-columns: auto minmax(0, 1fr); width: 20px; }
		@supports (line-break: anywhere) { #feature { line-break: anywhere; } }
	</style>
	<div id="normal" class="probe">abcdefghijklmnop</div>
	<div id="feature" class="probe"><a id="wrapped" href="#normal">abcdefghijklmnop</a></div>
	<div id="nowrap" class="probe anywhere">qwertyuiopasdf<br>qwertyuiopasdf</div>
	<div id="clusters" class="anywhere">e&#769;e&#769;e&#769;</div>
	<div id="spaces" class="anywhere">AAA B</div>
	<div class="grid"><span id="normal_track">abcdefghijklmnop</span><i></i></div>
	<div class="grid anywhere"><span id="anywhere_track">abcdefghijklmnop</span><i></i></div>
	<div id="responsive" class="anywhere"><span id="long_token">ABCDEFGHIJKLMNO</span></div>
	"""
	if layout.parse(source) != OK or layout.layout(Vector2i(100, 700)) != OK:
		_check(false, "Prepare anywhere line-break fixture: " + layout.get_render_error())
		return
	var normal: Rect2 = _rect(layout, "normal")
	var wrapped: HTMLElement = layout.get_element_by_id("wrapped")
	var rects: Array[Rect2] = wrapped.get_client_rects()
	_check(rects.size() == 4 and is_equal_approx(_rect(layout, "feature").size.y, normal.size.y * 4.0),
		"Anywhere wraps a long word into four lines")
	for rect: Rect2 in rects:
		_check(layout.get_link_at(rect.get_center()) == "#normal", "Every wrapped link fragment is hittable")
	_check(is_equal_approx(_rect(layout, "nowrap").size.y, normal.size.y * 2.0),
		"Nowrap still honors the forced break")
	_check(_rect(layout, "anywhere_track").size.x < _rect(layout, "normal_track").size.x,
		"Anywhere contributes grapheme boundaries to Grid minimum sizing")
	var plain: String = layout.get_plain_text()
	var start: int = plain.find("abcdefghijklmnop", 16)
	var last: Array[Rect2] = layout.get_selection_rects(start + 15, start + 16)
	_check(start == 17 and last.size() == 1, "Soft wrapping preserves the source and the last letter is selectable")
	if last.size() == 1:
		_check(layout.hit_test_text(Vector2(last[0].end.x - 0.01, last[0].get_center().y)) == start + 16,
			"The last painted fragment maps back to its source offset")
	var combined: int = plain.find("e\u0301e\u0301e\u0301")
	var first: Array[Rect2] = layout.get_selection_rects(combined, combined + 2)
	var second: Array[Rect2] = layout.get_selection_rects(combined + 2, combined + 4)
	_check(combined >= 0 and first.size() == 1 and second.size() == 1 and first[0].position.y < second[0].position.y,
		"Each accented grapheme stays together across line breaks")
	var spaces: int = plain.find("AAA B")
	var space_rects: Array[Rect2] = layout.get_selection_rects(spaces + 3, spaces + 4)
	_check(space_rects.size() == 1 and space_rects[0].position.y > _rect(layout, "spaces").position.y + 20.0,
		"Break-spaces permits wrapping before the first space")
	var narrow: Rect2 = _rect(layout, "responsive")
	_check(layout.layout(Vector2i(350, 700)) == OK and narrow.size.y > _rect(layout, "responsive").size.y,
		"Reflow merges text fragments again at a wider viewport")
	_check(layout.get_plain_text() == plain, "Viewport-only reflow preserves source text offsets")
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(140, 90)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(viewport.size)
	label.text = """<style>
		body { margin: 0; font: 20px/24px monospace; color: white; background: black; }
		div { width: 5.5ch; line-break: anywhere; text-shadow: 72px 0 magenta; }
	</style><div>MMMMMMMMMMMM</div>"""
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	var ink_bounds: Callable = func(region: Rect2i, shadow: bool) -> Rect2i:
		var ink: Rect2i
		for y: int in range(region.position.y, region.end.y):
			for x: int in range(region.position.x, region.end.x):
				var color: Color = image.get_pixel(x, y)
				var visible: bool = color.r >= 0.25 and color.b >= 0.25
				if visible and (color.g <= 0.07 if shadow else color.g >= 0.25):
					var pixel: Rect2i = Rect2i(x, y, 1, 1)
					ink = ink.merge(pixel) if ink.has_area() else pixel
		return ink
	for row: int in range(3):
		var glyphs: Rect2i = ink_bounds.call(Rect2i(0, row * 24, 69, 24), false)
		var shadow: Rect2i = ink_bounds.call(Rect2i(71, row * 24, 69, 24), true)
		_check(glyphs.has_area() and shadow.has_area() and absi(glyphs.size.x - shadow.size.x) <= 2,
			"Line %d paints both the shaped fragment and its shadow: %s and %s" % [row + 1, glyphs, shadow])
	viewport.queue_free()


func _check_overflow_wrap() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var source: String = """<!doctype html><style>
		body { margin: 0; font: 20px/24px monospace; }
		.probe { width: 5.5ch; }
		.anywhere { overflow-wrap: anywhere; }
		.break_word { overflow-wrap: break-word; }
		.legacy { word-break: break-word; overflow-wrap: normal; }
		.priority { width: 8.5ch; }
		.grid { display: grid; width: 20px; grid-template-columns: auto minmax(0, 1fr); }
		.preferred_grid { display: inline-grid; grid-template-columns: auto; }
		.fixed_grid { display: grid; grid-template-columns: 5.5ch; }
		@supports (word-wrap: anywhere) { #supported { overflow-wrap: anywhere; } }
	</style>
	<div class="probe" id="ow_normal">abcdefghijklmnop</div>
	<div class="probe anywhere" id="ow_anywhere"><a id="ow_link" href="#ow_normal">abcdefghijklmnop</a></div>
	<div class="probe break_word" id="ow_break_word">abcdefghijklmnop</div>
	<div class="probe legacy" id="ow_legacy">abcdefghijklmnop</div>
	<div class="probe" id="supported">abcdefghijklmnop</div>
	<div class="probe" id="alias" style="--value: break-word; overflow-wrap: normal;
		word-wrap: var(--value) !important; overflow-wrap: normal">abcdefghijklmnop</div>
	<div class="probe" id="alias_reset" style="word-wrap: anywhere; overflow-wrap: normal">abcdefghijklmnop</div>
	<div class="probe" id="alias_invalid" style="overflow-wrap: anywhere; word-wrap: bogus">abcdefghijklmnop</div>
	<div class="probe anywhere" id="ow_nowrap" style="white-space: nowrap">abcdefghijklmnop</div>
	<div class="priority anywhere"><span id="ow_prefix">abcd</span> <span id="ow_next">ghijkl</span></div>
	<div class="priority break_word"><span id="ow_long_prefix">abcd</span> <span id="ow_long">ghijklmnopqr</span></div>
	<div class="priority" style="line-break: anywhere"><span id="lb_prefix">abcd</span> <span id="lb_next">ghijkl</span></div>
	<div class="grid"><span id="ow_track_normal">abcdefghijklmnop</span><i></i></div>
	<div class="grid anywhere"><span id="ow_track_anywhere">abcdefghijklmnop</span><i></i></div>
	<div class="grid break_word"><span id="ow_track_break_word">abcdefghijklmnop</span><i></i></div>
	<div class="grid legacy"><span id="ow_track_legacy">abcdefghijklmnop</span><i></i></div>
	<div class="preferred_grid anywhere"><span id="ow_track_preferred">abcdefghijklmnop</span></div>
	<div class="fixed_grid break_word"><span id="ow_track_fixed">abcdefghijklmnop</span></div>
	"""
	if layout.parse(source) != OK or layout.layout(Vector2i(420, 1100)) != OK:
		_check(false, "Prepare overflow-wrap fixture: " + layout.get_render_error())
		return
	var line_height: float = _rect(layout, "ow_normal").size.y
	for id: String in ["ow_anywhere", "ow_break_word", "ow_legacy", "supported", "alias", "alias_invalid"]:
		_check(is_equal_approx(_rect(layout, id).size.y, line_height * 4.0),
			"%s uses the grapheme fallback" % id)
	for id: String in ["alias_reset", "ow_nowrap"]:
		_check(is_equal_approx(_rect(layout, id).size.y, line_height),
			"%s keeps an unbroken line" % id)
	_check(_has_diagnostic(layout, "word-wrap", "bogus", "inline_style"), "Alias diagnostics keep the declared spelling")
	var next_word: HTMLElement = layout.get_element_by_id("ow_next")
	_check(next_word.get_client_rects().size() == 1 and
		next_word.get_bounding_rect().position.y > _rect(layout, "ow_prefix").position.y,
		"An ordinary space moves the entire next word onto the next line")
	var long_word: HTMLElement = layout.get_element_by_id("ow_long")
	_check(long_word.get_client_rects().size() == 2 and
		long_word.get_bounding_rect().position.y > _rect(layout, "ow_long_prefix").position.y,
		"An overlong word uses the ordinary break and then falls back on a fresh line")
	_check(is_equal_approx(_rect(layout, "lb_next").position.y, _rect(layout, "lb_prefix").position.y),
		"Line-break anywhere treats a word boundary and grapheme boundaries equally")
	var normal_width: float = _rect(layout, "ow_track_normal").size.x
	var anywhere_width: float = _rect(layout, "ow_track_anywhere").size.x
	_check(anywhere_width < normal_width and is_equal_approx(_rect(layout, "ow_track_legacy").size.x, anywhere_width),
		"Anywhere and legacy word-break break-word reduce the minimum content width")
	var break_word_width: float = _rect(layout, "ow_track_break_word").size.x
	_check(is_equal_approx(break_word_width, normal_width),
		"Break-word preserves minimum content width: %s versus %s" % [break_word_width, normal_width])
	var preferred_width: float = _rect(layout, "ow_track_preferred").size.x
	_check(is_equal_approx(preferred_width, normal_width),
		"An inline grid uses the full preferred width with anywhere: %s versus %s" % [preferred_width, normal_width])
	_check(is_equal_approx(_rect(layout, "ow_track_fixed").size.y, line_height * 4.0),
		"Break-word still wraps once Grid assigns the final track width")
	var link: HTMLElement = layout.get_element_by_id("ow_link")
	for fragment: Rect2 in link.get_client_rects():
		_check(layout.get_link_at(fragment.get_center()) == "#ow_normal", "Each fallback line keeps the link hit area")
	var plain: String = layout.get_plain_text()
	_check(plain.contains("abcdefghijklmnop"), "Fallback wrapping preserves source text without a hyphen")
	var row: HTMLElement = layout.get_element_by_id("ow_anywhere")
	_check(row.set_attribute("style", "overflow-wrap: normal") == OK and layout.layout(Vector2i(420, 1100)) == OK,
		"Remove overflow break positions through the DOM")
	_check(is_equal_approx(row.get_bounding_rect().size.y, line_height) and layout.get_plain_text() == plain,
		"Reflow removes fallback lines and preserves source offsets")


func _check_word_break_all() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	var source: String = """<!doctype html><style>
		body { margin: 0; font: 20px/24px monospace; }
		.probe { width: 2.5ch; }
		.break_all { word-break: var(--mode, break-all); }
		.grid { display: grid; grid-template-columns: auto minmax(0, 1fr); width: 20px; }
		@supports (word-break: break-all) { #wb_supported { word-break: break-all; } }
	</style>
	<div class="probe" id="wb_normal">abcdef</div>
	<div class="probe break_all" id="wb_letters">abcdef</div>
	<div class="probe break_all" id="wb_numbers">123456</div>
	<div class="probe" id="wb_supported">abcdef</div>
	<div class="probe break_all" id="wb_nowrap" style="white-space: nowrap">abcdef</div>
	<div class="probe break_all" id="wb_open">a<span id="wb_opening">(</span><span id="wb_opened">b</span></div>
	<div class="probe break_all" id="wb_close">a<span id="wb_closed">b</span><span id="wb_closing">)</span>c</div>
	<div class="probe break_all">a<span id="wb_money">$</span><span id="wb_amount">1</span></div>
	<div class="probe break_all">a<span id="wb_fraction">1</span><span id="wb_percent">%</span></div>
	<div class="probe break_all">x<span id="wb_glue_left">A</span>&#x2011;<span id="wb_glue_right">B</span></div>
	<div class="probe break_all" style="width: 2.1ch; line-break: loose"><span id="wb_loose_left">AB</span><span id="wb_loose_hyphen">‐</span>C</div>
	<div class="probe break_all" style="width: 2.1ch; line-break: strict">A<span id="wb_strict_left">B</span><span id="wb_strict_hyphen">‐</span>C</div>
	<div class="probe break_all" style="width: 1.2ch" id="wb_accents">e&#769;e&#769;e&#769;</div>
	<div class="probe break_all" id="wb_thai">กขคงจฉชซ</div>
	<div class="probe break_all" id="wb_arabic" style="font-family: Arial">ببببببببب</div>
	<div class="probe break_all" style="width: 8.5ch"><span id="wb_prefix">abcd</span> <a id="wb_link" href="#wb_normal">ghijkl</a></div>
	<div class="grid"><span id="wb_track_normal">abcdef</span><i></i></div>
	<div class="grid break_all"><span id="wb_track_break">abcdef</span><i></i></div>
	"""
	if layout.parse(source) != OK or layout.layout(Vector2i(320, 1000)) != OK:
		_check(false, "Prepare word-break break-all fixture: " + layout.get_render_error())
		return
	var line_height: float = _rect(layout, "wb_normal").size.y
	for id: String in ["wb_letters", "wb_numbers", "wb_supported", "wb_accents"]:
		_check(is_equal_approx(_rect(layout, id).size.y, line_height * 3.0),
			"%s wraps into three lines" % id)
	_check(is_equal_approx(_rect(layout, "wb_nowrap").size.y, line_height),
		"Nowrap suppresses break-all opportunities")
	var punctuation: Dictionary[String, String] = {
		"wb_opening": "wb_opened",
		"wb_closed": "wb_closing",
		"wb_money": "wb_amount",
		"wb_fraction": "wb_percent",
		"wb_glue_left": "wb_glue_right",
		"wb_strict_left": "wb_strict_hyphen",
	}
	for id: String in punctuation:
		_check(is_equal_approx(_rect(layout, id).position.y, _rect(layout, punctuation[id]).position.y),
			"%s and %s stay on the same line" % [id, punctuation[id]])
	_check(_rect(layout, "wb_loose_hyphen").position.y > _rect(layout, "wb_loose_left").position.y,
		"Loose line breaking allows a hyphen to start the next line after remapping letters")
	var link: HTMLElement = layout.get_element_by_id("wb_link")
	var fragments: Array[Rect2] = link.get_client_rects()
	_check(fragments.size() == 2 and is_equal_approx(_rect(layout, "wb_link").position.y, _rect(layout, "wb_prefix").position.y),
		"Break-all can use a word-internal break on the same line as the previous word")
	for fragment: Rect2 in fragments:
		_check(layout.get_link_at(fragment.get_center()) == "#wb_normal", "A break-all fragment retains its link")
	_check(_rect(layout, "wb_track_break").size.x < _rect(layout, "wb_track_normal").size.x,
		"Break-all contributes word-internal breaks to the minimum content width")
	var plain: String = layout.get_plain_text()
	var offset: int = plain.find("e\u0301e\u0301e\u0301")
	var accent_bounds: Array[Rect2] = layout.get_selection_rects(offset, offset + 2)
	_check(offset >= 0 and accent_bounds.size() == 1,
		"A selected accent and its base occupy one visible grapheme")
	for id: String in ["wb_arabic", "wb_thai"]:
		var element: HTMLElement = layout.get_element_by_id(id)
		_check(element.get_bounding_rect().size.y > line_height and plain.contains(element.get_text_content()),
			"%s wraps without changing copied text" % id)


func _check_lexical_word_breaks() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	var samples: Dictionary[String, String] = {
		"th": "ภาษาไทยสวยงามมาก",
		"lo": "ພາສາລາວເປັນພາສາ",
		"km": "ភាសាខ្មែរជាភាសាមួយ",
		"my": "မြန်မာဘာသာစကား",
	}
	var first_words: Dictionary[String, String] = {
		"th": "ภาษา",
		"lo": "ພາສາ",
		"km": "ភាសាខ្មែរ",
		"my": "မြန်မာဘာသာ",
	}
	var source: String = """<!doctype html><style>
		body { margin: 0; font-size: 24px; line-height: 30px; }
		.probe { width: 350px; }
		.manual { word-break: manual; }
		.lexical_grid { display: grid; grid-template-columns: auto minmax(0, 1fr); width: 20px; }
		@supports (word-break: manual) { #manual_support { word-break: manual; } }
	</style>"""
	for language: String in samples:
		source += ('<section lang="%s"><div id="%s_normal" class="probe">%s</div>' +
			'<div id="%s_manual" class="probe manual">%s</div><span id="%s_first">%s</span></section>') % [
			language, language, samples[language], language, samples[language], language, first_words[language],
		]
	source += """<article lang="th-TH">
		<div class="probe" id="markup"><span id="split_word_start">ภา</span><span id="split_word">ษา</span><a id="next_word" href="#th_normal">ไทย</a></div>
		<div class="probe" id="manual_support">ภาษาไทยสวยงามมาก</div>
		<div class="probe manual" id="explicit_wbr">ภาษา<wbr>ไทย</div>
		<div class="probe manual" id="explicit_zwsp">ภาษา&#x200b;ไทย</div>
		<div class="probe" id="single_word" style="width: 1px">ภาษา</div>
		<div class="manual" id="manual_kana" style="width: 1.5em">カタカナ</div>
		<div class="lexical_grid"><span id="lexical_track">ภาษาไทยสวยงามมาก</span><i></i></div>
		<div class="lexical_grid manual"><span id="manual_track">ภาษาไทยสวยงามมาก</span><i></i></div>
	</article>
	<div id="fallback_script" style="width: 1px">ᨠᨡᨢᨣ</div>
	<div id="fallback_manual" class="manual" style="width: 1px">ᨠᨡᨢᨣ</div>"""
	if layout.parse(source) != OK or layout.layout(Vector2i(1100, 1600)) != OK:
		_check(false, "Prepare lexical line breaking fixture: " + layout.get_render_error())
		return
	var word_widths: Dictionary[String, float] = {}
	for language: String in samples:
		word_widths[language] = _rect(layout, language + "_first").size.x
		_check(word_widths[language] > 0.0, "Measure the first %s word before editing" % language)
	for language: String in samples:
		for suffix: String in ["_normal", "_manual"]:
			var probe: HTMLElement = layout.get_element_by_id(language + suffix)
			_check(probe.set_attribute("style", "width: %.3fpx" % (word_widths[language] + 0.25)) == OK,
				"Set the line width to the first %s word" % language)
	var thai_width: float = word_widths["th"]
	var markup: HTMLElement = layout.get_element_by_id("markup")
	_check(markup.set_attribute("style", "width: %.3fpx" % (thai_width - 0.25)) == OK, "Narrow the markup fixture")
	for id: String in ["manual_support", "explicit_wbr", "explicit_zwsp"]:
		_check(layout.get_element_by_id(id).set_attribute("style", "width: %.3fpx" % (thai_width + 0.25)) == OK,
			"Set the line width for " + id)
	_check(layout.layout(Vector2i(1100, 1600)) == OK, "Reflow with the lexical widths")
	var plain: String = layout.get_plain_text()
	for language: String in samples:
		var normal: Rect2 = _rect(layout, language + "_normal")
		var manual: Rect2 = _rect(layout, language + "_manual")
		_check(normal.size.y > manual.size.y, "%s uses dictionary breaks in normal mode" % language)
		var offset: int = plain.find(samples[language])
		var first: Array[Rect2] = layout.get_selection_rects(offset, offset + first_words[language].length())
		_check(offset >= 0 and first.size() == 1, "The first %s word stays on one line and is selectable" % language)
	var manual_height: float = _rect(layout, "th_manual").size.y
	_check(is_equal_approx(_rect(layout, "single_word").size.y, manual_height),
		"A known dictionary can return a single word without falling back to grapheme breaks")
	_check(is_equal_approx(_rect(layout, "manual_support").size.y, manual_height),
		"Word-break manual participates in feature queries")
	_check(_rect(layout, "explicit_wbr").size.y > manual_height and _rect(layout, "explicit_zwsp").size.y > manual_height,
		"Manual preserves explicit wbr and zero-width space opportunities")
	_check(_rect(layout, "manual_kana").size.y > manual_height, "Manual preserves ordinary Kana line breaks")
	_check(_rect(layout, "lexical_track").size.x < _rect(layout, "manual_track").size.x,
		"Grid minimum sizing uses dictionary boundaries")
	_check(_rect(layout, "fallback_script").size.y > _rect(layout, "fallback_manual").size.y,
		"A script without a dictionary can still wrap at grapheme boundaries")
	var word_start: Rect2 = _rect(layout, "split_word_start")
	var first_word: Rect2 = _rect(layout, "split_word")
	var link: Rect2 = _rect(layout, "next_word")
	_check(is_equal_approx(first_word.position.y, word_start.position.y) and
		link.position.y > first_word.position.y and layout.get_link_at(link.get_center()) == "#th_normal",
		"Dictionary lookup crosses inline markup and preserves the link: %s, %s, %s" % [word_start, first_word, link])
	_check(markup.get_text_content() == "ภาษาไทย", "Lexical fragments leave source text intact")
	_check(markup.set_attribute("style", "width: %.3fpx; word-break: manual" % (thai_width - 0.25)) == OK and
		layout.layout(Vector2i(1100, 1600)) == OK, "Switch the inline markup to manual mode")
	_check(is_equal_approx(_rect(layout, "next_word").position.y, _rect(layout, "split_word").position.y),
		"Changing to manual removes previously computed dictionary breaks")


func _check_discretionary_hyphenation() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var source: String = """<!doctype html><style>
		body { margin: 0; font: 20px/24px monospace; }
		.probe { width: 6ch; }
		.grid { display: grid; grid-template-columns: auto minmax(0, 1fr); width: 1px; }
	</style>
	<div id="reference" class="probe">abcdefgh</div>
	<div id="manual" class="probe"><a href="#reference">abc<span id="default_mark">&shy;</span>defgh</a></div>
	<div id="disabled" class="probe" style="hyphens: none"><span>abc&shy;defgh</span></div>
	<div id="nowrap" class="probe" style="white-space: nowrap">abc&shy;defgh</div>
	<div id="auto" class="probe" style="hyphens: auto">abc&shy;defgh</div>
	<div id="focused" class="probe" style="hyphenate-character: var(--suffix); --suffix: '=='">
		ab<span id="earlier_mark">&shy;</span>cde<span id="later_mark">&shy;</span>fgh</div>
	<div id="at_start" class="probe">&shy;abcdefgh</div>
	<div id="empty" class="probe" style="hyphenate-character: ''">abc<span id="empty_mark">&shy;</span>defgh</div>
	<div id="break_all" class="probe" style="word-break: break-all">abc<span id="break_all_mark">&shy;</span>defgh</div>
	<div id="anywhere" class="probe" style="line-break: anywhere">abc<span id="anywhere_mark">&shy;</span>defgh</div>
	<div id="oversize" class="probe" style="overflow-wrap: anywhere; hyphenate-character: '============'">
		abc<span id="oversize_mark">&shy;</span>defgh</div>
	<div class="probe" style="text-wrap: balance; hyphenate-character: '='">
		abc<span id="balanced_first">&shy;</span>de<span id="balanced_second">&shy;</span>fgh</div>
	<div class="grid" style="hyphenate-character: '=='"><span id="track">abc&shy;def</span><i></i></div>
	<div class="grid" style="hyphens: none"><span id="disabled_track">abc&shy;def</span><i></i></div>
	<div class="grid"><span id="reference_track">abcdef</span><i></i></div>"""
	if layout.parse(source) != OK or layout.layout(Vector2i(240, 640)) != OK:
		_check(false, "Prepare discretionary hyphenation fixture: " + layout.get_render_error())
		return
	var line_height: float = _rect(layout, "reference").size.y
	for id: String in ["disabled", "nowrap", "at_start"]:
		_check(is_equal_approx(_rect(layout, id).size.y, line_height), id + " keeps its original line")
	for id: String in ["manual", "auto", "empty", "break_all", "anywhere", "oversize"]:
		_check(is_equal_approx(_rect(layout, id).size.y, 2.0 * line_height), id + " wraps into two lines")
	for id: String in ["empty_mark", "break_all_mark", "anywhere_mark", "later_mark", "oversize_mark", "balanced_second"]:
		_check(_rect(layout, id).size.x == 0.0, id + " stays visually empty")
	var mark: Rect2 = _rect(layout, "default_mark")
	_check(mark.has_area() and layout.get_link_at(mark.get_center()) == "#reference",
		"The discretionary glyph occupies space inside the wrapped link")
	_check(_rect(layout, "earlier_mark").size.x > mark.size.x * 1.5,
		"Break selection accounts for the width of the full replacement string")
	_check(_rect(layout, "balanced_first").has_area(), "Balanced text can choose and paint an earlier hyphenation point")
	var minimum: float = _rect(layout, "track").size.x
	var unsplit: float = _rect(layout, "reference_track").size.x
	_check(is_equal_approx(minimum * 6.0, unsplit * 5.0), "Min-content includes the replacement glyphs: %s vs %s" % [minimum, unsplit])
	_check(is_equal_approx(_rect(layout, "disabled_track").size.x, unsplit), "Disabling hyphenation preserves the unsplit minimum")
	var text: String = layout.get_plain_text()
	var start: int = text.find("abc\u00addefgh")
	var before: Array[Rect2] = layout.get_selection_rects(start, start + 3)
	var after: Array[Rect2] = layout.get_selection_rects(start + 4, start + 9)
	_check(start >= 0 and before.size() == 1 and after.size() == 1 and after[0].position.y > before[0].position.y,
		"Selection follows both lines while the soft hyphen keeps its source offset")
	_check("==" not in text and "‐" not in text and layout.get_css_diagnostics().is_empty(),
		"Painted hyphens never become document text")

	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(400, 100)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(200, 100)
	label.text = """<!doctype html><style>
		body { margin: 0; font: 20px/24px monospace; color: white; background: black; }
		#paint { width: 6ch; hyphenate-character: '=='; }
		#paint_mark { color: lime; }
	</style><div id="paint">abc<span id="paint_mark">&shy;</span>defgh</div>"""
	viewport.add_child(label)
	var markdown: MarkdownLabel = MarkdownLabel.new()
	markdown.position = Vector2(200, 0)
	markdown.size = Vector2(200, 100)
	markdown.parse_html = true
	var icon_texture: ImageTexture = ImageTexture.create_from_image(Image.create_empty(1, 1, false, Image.FORMAT_RGBA8))
	for icon: StringName in [&"task_checked", &"task_unchecked", &"file_broken"]:
		markdown.add_theme_icon_override(icon, icon_texture)
	markdown.text = """<div>
<style>
body { margin: 0; background: black; color: white; }
.measure { font: 20px/24px monospace; width: 6ch; hyphenate-character: '=='; }
.mark { color: lime; }
</style>
<div class="measure">abc<span class="mark">&shy;</span>defgh</div>
</div>"""
	viewport.add_child(markdown)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	var marker_ink: Callable = func(bounds: Rect2i) -> bool:
		for y: int in range(bounds.position.y, bounds.end.y):
			for x: int in range(bounds.position.x, bounds.end.x):
				var pixel: Color = image.get_pixel(x, y)
				if pixel.g > 0.3 and pixel.r < 0.05 and pixel.b < 0.05:
					return true
		return false
	_check(marker_ink.call(Rect2i(_label_rect(label, "paint_mark"))),
		"HTMLLabel paints the inherited custom hyphen in the marker's foreground color")
	_check(marker_ink.call(Rect2i(200, 0, 200, 100)), "Markdown HTML blocks inherit and paint the same hyphen")
	_check(label.get_element_by_id("paint").set_attribute("style", "width: 12ch") == OK, "Widen the discretionary fixture")
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	_check(_label_rect(label, "paint_mark").size.x == 0.0, "Reflow removes the painted hyphen when the word fits")
	viewport.queue_free()


func _check_hyphenation_limits() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var source: String = """<!doctype html><style>
		body { margin: 0; font: 20px/24px monospace; hyphenate-character: '='; }
		.zone { width: 10ch; }
		.six { width: 6ch; }
		.balance { width: 9ch; text-wrap: balance; }
		.grid { display: grid; grid-template-columns: auto minmax(0, 1fr); width: 1px; }
		@supports (hyphenate-limit-zone: calc(5% + 1ch)) and (hyphenate-limit-lines: 0) {
		  #conditional { hyphenate-limit-lines: calc(1 - 1); }
		}
	</style>
	<div class="zone" id="ten_characters">aaaaaa bb<span id="default_zone">&shy;</span>cccccc</div>
	<div id="zoned" style="hyphenate-limit-zone: 50%">
	  <div class="zone">aaaaaa bb<span id="large_zone">&shy;</span>cccccc</div>
	</div>
	<div class="zone" style="hyphenate-limit-zone: calc(25% + 1ch)">
	  aaaaaa bb<span id="small_zone">&shy;</span>cccccc</div>
	<div class="zone" style="text-wrap: balance; hyphenate-limit-zone: 65%">
	  aaaa bbb<span id="balanced_zone">&shy;</span>ccccccc</div>
	<div class="zone" style="hyphenate-limit-zone: 50%">
	  <i style="float: left; width: 3ch; height: 24px"></i>aaa bb<span id="float_zone">&shy;</span>cccccc</div>
	<div class="six" style="hyphenate-limit-zone: 100%">abc<span id="no_ordinary">&shy;</span>defgh</div>
	<div class="six" style="hyphenate-limit-lines: no-limit">
	  abcd<span id="unlimited_first">&shy;</span>efgh<span id="unlimited_second">&shy;</span>ijkl</div>
	<div style="hyphenate-limit-lines: var(--limit); --limit: 1">
	  <div class="six">abcd<span id="limited_first">&shy;</span>efgh<span id="limited_second">&shy;</span>ijkl<br>
	    mnop<span id="after_forced">&shy;</span>qrst</div>
	  <div class="six">abcd<span id="natural_first">&shy;</span>efgh xyz<span id="after_ordinary">&shy;</span>mnop</div>
	</div>
	<div class="six" style="hyphenate-limit-lines: 1; hyphenate-character: ''">
	  <span id="empty_head">abcd</span>&shy;<span id="empty_middle">efgh</span>&shy;<span id="empty_end">ijkl</span></div>
	<div class="six" id="conditional">abcd<span id="zero_limit">&shy;</span>efgh</div>
	<div class="balance">aaaa bbb<span id="balanced_unlimited_a">&shy;</span>ccc
	  dd<span id="balanced_unlimited_b">&shy;</span>eeee ff</div>
	<div class="balance" style="hyphenate-limit-lines: 1">aaaa bbb<span id="balanced_limited_a">&shy;</span>ccc
	  dd<span id="balanced_limited_b">&shy;</span>eeee ff</div>
	<div class="grid" style="hyphenate-limit-zone: 100%; hyphenate-limit-lines: 1">
	  <span id="normal_minimum">abc&shy;def</span><i></i></div>
	<div class="grid" style="hyphenate-limit-lines: 0; hyphenate-limit-lines: 1.5">
	  <span id="disabled_minimum">abc&shy;def</span><i></i></div>"""
	if layout.parse(source) != OK or layout.layout(Vector2i(260, 880)) != OK:
		_check(false, "Prepare hyphenation limit fixture: " + layout.get_render_error())
		return
	for id: String in ["default_zone", "small_zone", "float_zone", "no_ordinary", "unlimited_first", "unlimited_second",
		"limited_first", "after_forced", "natural_first", "after_ordinary", "balanced_unlimited_a", "balanced_unlimited_b"]:
		_check(_rect(layout, id).has_area(), id + " permits the discretionary glyph")
	for id: String in ["large_zone", "balanced_zone", "limited_second", "zero_limit", "balanced_limited_a", "balanced_limited_b"]:
		_check(_rect(layout, id).size.x == 0.0, id + " suppresses the discretionary glyph")
	var empty_middle_y: float = _rect(layout, "empty_middle").position.y
	_check(empty_middle_y > _rect(layout, "empty_head").position.y and
		is_equal_approx(empty_middle_y, _rect(layout, "empty_end").position.y),
		"A break with an empty replacement still consumes one hyphenated line")
	_check(_rect(layout, "normal_minimum").size.x < _rect(layout, "disabled_minimum").size.x,
		"Line-relative limits preserve applicable minimum-content opportunities")
	_check(_has_diagnostic(layout, "hyphenate-limit-lines", "1.5", "inline_style"), "A fractional literal is invalid")
	var ten_characters: float = _rect(layout, "ten_characters").size.x
	var container: HTMLElement = layout.get_element_by_id("zoned")
	_check(container.set_attribute("style", "hyphenate-limit-zone: 0") == OK and layout.layout(Vector2i(260, 880)) == OK and
		_rect(layout, "large_zone").has_area(), "Updating an inherited zone recomputes the line breaks")

	var responsive: HTMLLayout = HTMLLayout.new()
	var responsive_source: String = """<!doctype html><style>
		body { margin: 0; font: 20px/24px monospace; hyphenate-limit-zone: 45%; }
	</style><div>aaaaaa bb<span id="responsive_mark">&shy;</span>cccccc</div>"""
	_check(responsive.parse(responsive_source) == OK and responsive.layout(Vector2i(roundi(ten_characters), 96)) == OK,
		"Lay out the percentage zone at ten character widths")
	var responsive_mark: HTMLElement = responsive.get_element_by_id("responsive_mark")
	_check(responsive_mark != null and responsive_mark.get_bounding_rect().size.x == 0.0,
		"The percentage zone permits leaving four character widths empty")
	_check(responsive.layout(Vector2i(roundi(ten_characters * 1.2), 96)) == OK and responsive_mark != null and
		responsive_mark.get_bounding_rect().has_area(), "Resizing resolves the percentage against the new line width")


func _check_hyphenation_character_limits() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var source: String = """<!doctype html><style>
		body { margin: 0; font: 20px/24px monospace; hyphenate-character: '='; }
		.probe { width: 6ch; }
		.grid { display: grid; grid-template-columns: auto minmax(0, 1fr); width: 1px; }
		@supports (hyphenate-limit-chars: auto calc(2.5) auto) { #supports { hyphenate-limit-chars: auto calc(2.5); } }
	</style>
	<div class="probe" style="hyphenate-limit-chars: 8">abc<span id="equal_total">&shy;</span>defgh</div>
	<div class="probe" style="hyphenate-limit-chars: 9">abc<span id="short_total">&shy;</span>defgh</div>
	<div class="probe" style="hyphenate-limit-chars: 8 4">abcd<span id="symmetric_fit">&shy;</span>efgh</div>
	<div class="probe" style="hyphenate-limit-chars: auto 4">abcde<span id="symmetric_short">&shy;</span>fgh</div>
	<div class="probe" style="hyphenate-limit-chars: 9 4 5"><b>ab</b><span>cd</span><span id="inline_word">&shy;</span><a href="#supports">efghi</a></div>
	<div class="probe" style="hyphenate-limit-chars: 9 5 4">abcd<span id="short_before">&shy;</span>efghi</div>
	<div class="probe" style="hyphenate-limit-chars: 9 4 6">abcd<span id="short_after">&shy;</span>efghi</div>
	<div class="probe" style="hyphenate-limit-chars: 6 auto 3">a<span id="auto_prefix">&shy;</span>bcdefgh</div>
	<div class="probe" style="hyphenate-limit-chars: 0 1 1">a<span id="one_character">&shy;</span>bcdefgh</div>
	<div class="probe" style="hyphenate-limit-chars: auto 4 3">a&#769;bc<span id="combining_short">&shy;</span>defg</div>
	<div class="probe" style="hyphenate-limit-chars: auto 3 3">a&#769;bc<span id="combining_fit">&shy;</span>defg</div>
	<div class="probe" style="hyphenate-limit-chars: 0 1 1">a<span id="inside_grapheme">&shy;</span>&#769;bcdefg</div>
	<div class="probe" lang="en" style="hyphenate-limit-chars: auto 4 3">d’ef<span id="punctuation_short">&shy;</span>ghijk</div>
	<div class="probe" lang="en" style="hyphenate-limit-chars: auto 3 3">d’ef<span id="punctuation_fit">&shy;</span>ghijk</div>
	<div class="probe" style="hyphenate-limit-chars: 8 4 4">1.234<span id="decimal_fit">&shy;</span>5678</div>
	<div class="probe" style="hyphenate-limit-chars: 8 4 4">ab_cd<span id="connector_fit">&shy;</span>efgh</div>
	<div style="width: 8ch; hyphenate-limit-chars: 9 2 2">zzzz,ab<span id="separate_word">&shy;</span>cdefgh</div>
	<div class="probe" id="supports">abcde<span id="supported_math">&shy;</span>fg</div>
	<div class="probe" id="inherited_limit" style="hyphenate-limit-chars: 9; --limit: 8 3 5">
	  abc<span id="inherited_mark">&shy;</span>defgh<br>abc<span style="hyphenate-limit-chars: var(--limit)" id="marker_override">&shy;</span>defgh</div>
	<div class="probe" style="hyphenate-limit-chars: 9; hyphenate-limit-chars: 8.5">abc<span id="invalid_minimum">&shy;</span>defgh</div>
	<div class="grid" style="hyphenate-limit-chars: 7"><span id="minimum_with_cut">abc&shy;defg</span><i></i></div>
	<div class="grid" style="hyphenate-limit-chars: 8"><span id="minimum_without_cut">abc&shy;defg</span><i></i></div>"""
	if layout.parse(source) != OK or layout.layout(Vector2i(260, 1000)) != OK:
		_check(false, "Prepare hyphenation character limits: " + layout.get_render_error())
		return
	for id: String in ["equal_total", "symmetric_fit", "inline_word", "one_character", "combining_fit",
		"punctuation_fit", "decimal_fit", "connector_fit", "marker_override"]:
		_check(_rect(layout, id).has_area(), id + " meets all character limits")
	for id: String in ["short_total", "symmetric_short", "short_before", "short_after", "auto_prefix", "combining_short",
		"inside_grapheme", "punctuation_short", "separate_word", "supported_math", "inherited_mark", "invalid_minimum"]:
		_check(_rect(layout, id).size.x == 0.0, id + " fails a character limit")
	_check(_has_diagnostic(layout, "hyphenate-limit-chars", "8.5", "inline_style"), "A decimal literal is invalid")
	_check(_rect(layout, "minimum_with_cut").size.x < _rect(layout, "minimum_without_cut").size.x,
		"The character limit also applies to intrinsic sizing")
	var container: HTMLElement = layout.get_element_by_id("inherited_limit")
	_check(container.set_attribute("style", "hyphenate-limit-chars: initial; --limit: 8 3 5") == OK and
		layout.layout(Vector2i(260, 1000)) == OK and _rect(layout, "inherited_mark").has_area(),
		"Changing the inherited limit updates the available break on the same element")


func _check_hyphenation_last_line() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	layout.css_diagnostics_enabled = true
	var source: String = """<!doctype html><style>
		body { margin: 0; font: 20px/24px monospace; hyphenate-character: '='; }
		.ten { width: 10ch; }
		.six { width: 6ch; }
		.protected { hyphenate-limit-last: always; }
		.grid { display: grid; grid-template-columns: auto minmax(0, 1fr); width: 1px; }
		@supports (hyphenate-limit-last: spread) { #conditional { hyphenate-limit-last: always; } }
	</style>
	<div class="ten">aaaaaa bb<span id="ordinary_mark">&shy;</span>cccccc</div>
	<div class="protected" id="inherited">
	  <div class="ten">aaaaaa <a id="moved_link" href="#conditional">bb<span id="limited_mark">&shy;</span>cccccc</a></div>
	  <div class="ten" style="hyphenate-limit-last: none">aaaaaa bb<span id="none_mark">&shy;</span>cccccc</div>
	</div>
	<div class="ten" id="conditional">aaaaaa bb<span id="supported_mark">&shy;</span>cccccc</div>
	<div class="ten" style="hyphenate-limit-last: column">aaaaaa bb<span id="column_mark">&shy;</span>cccccc</div>
	<div class="ten" style="hyphenate-limit-last: page">aaaaaa bb<span id="page_mark">&shy;</span>cccccc</div>
	<div class="ten" style="hyphenate-limit-last: spread">aaaaaa bb<span id="spread_mark">&shy;</span>cccccc</div>
	<div class="ten protected">
	  aaaaaa bb<span id="early_group">&shy;</span>cccccc<br>dddddd ee<span id="final_group">&shy;</span>ffffff</div>
	<div class="ten protected">
	  aaaaaa bb<span id="early_inline">&shy;</span>cccccc<div>middle</div>dddddd ee<span id="final_inline">&shy;</span>ffffff</div>
	<div class="ten protected" id="extended_tail">
	  aaaaa bbb<span id="first_trial">&shy;</span>ccc dd<span id="second_trial">&shy;</span>eeee</div>
	<div class="ten protected" style="text-wrap: balance">aaaa bbb<span id="balanced_last">&shy;</span>ccccccc</div>
	<div style="width: 11ch; text-wrap: balance">aaaa bbbb cccc ddd<span id="balance_adds_hyphen">&shy;</span>ee ff</div>
	<div class="protected" style="width: 11ch; text-wrap: balance">aaaa bbbb cccc ddd<span id="balance_respects_last">&shy;</span>ee ff</div>
	<div class="ten protected">
	  <i id="last_float" style="float: left; width: 2ch; height: 48px"></i>
	  aaa <b id="floated_word">bb<span id="floated_mark">&shy;</span>cccccc</b></div>
	<div class="six protected">abcd<span id="overflow_fallback">&shy;</span>efgh</div>
	<div class="six protected" style="overflow-wrap: anywhere">abcd<span id="emergency_wrap">&shy;</span>efgh</div>
	<div class="ten protected" style="hyphenate-limit-last: unknown">aaaaaa bb<span id="invalid_last">&shy;</span>cccccc</div>
	<div class="grid"><span id="normal_intrinsic">abc&shy;defg</span><i></i></div>
	<div class="grid protected"><span id="limited_intrinsic">abc&shy;defg</span><i></i></div>"""
	if layout.parse(source) != OK or layout.layout(Vector2i(260, 1080)) != OK:
		_check(false, "Prepare last-line hyphenation limits: " + layout.get_render_error())
		return
	for id: String in ["ordinary_mark", "none_mark", "column_mark", "page_mark", "spread_mark", "early_group",
		"early_inline", "balance_adds_hyphen", "overflow_fallback"]:
		_check(_rect(layout, id).has_area(), id + " keeps its break opportunity")
	for id: String in ["limited_mark", "supported_mark", "final_group", "final_inline", "first_trial", "second_trial",
		"balanced_last", "balance_respects_last", "floated_mark", "emergency_wrap", "invalid_last"]:
		_check(_rect(layout, id).size.x == 0.0, id + " avoids painting on the last full line")
	var link: HTMLElement = layout.get_element_by_id("moved_link")
	var link_boxes: Array[Rect2] = link.get_client_rects()
	_check(link_boxes.size() == 1 and layout.get_link_at(link_boxes[0].get_center()) == "#conditional" and
		link.get_text_content() == "bb\u00adcccccc", "The replay moves the entire linked word and preserves its source text")
	_check(is_equal_approx(_rect(layout, "extended_tail").size.y, 72.0), "A longer tail protects its newly penultimate line")
	var floated: Rect2 = _rect(layout, "last_float")
	var word: Rect2 = _rect(layout, "floated_word")
	_check(floated.has_area() and is_equal_approx(word.position.x, floated.end.x) and word.position.y > floated.position.y + 20.0,
		"Replaying the terminal lines preserves the float and the remaining width beside it")
	_check(is_equal_approx(_rect(layout, "normal_intrinsic").size.x, _rect(layout, "limited_intrinsic").size.x),
		"Last-line protection leaves intrinsic sizing opportunities available")
	_check(_has_diagnostic(layout, "hyphenate-limit-last", "unknown", "inline_style"), "An invalid literal keeps the earlier rule")
	var container: HTMLElement = layout.get_element_by_id("inherited")
	_check(container.set_attribute("style", "--end: none; hyphenate-limit-last: var(--end)") == OK and
		layout.layout(Vector2i(260, 1080)) == OK and _rect(layout, "limited_mark").has_area(),
		"The last-line limit follows inherited variable updates without stale line state")


func _check_automatic_hyphenation() -> void:
	var layout: HTMLLayout = HTMLLayout.new()
	var source: String = """<!doctype html><style>
		body { margin: 0; font: 20px/24px monospace; hyphenate-character: '='; }
		.probe { width: 8ch; hyphens: auto; }
		.grid { display: grid; grid-template-columns: auto minmax(0, 1fr); width: 1px; }
	</style>
	<div lang="en-US" class="probe" id="american">dictionary</div>
	<div lang="en-Latn-GB" class="probe" id="british">dictionary</div>
	<div class="probe" id="unknown_language">dictionary</div>
	<div lang="en-Cyrl" class="probe" id="other_script">dictionary</div>
	<div lang="en-US" class="probe" id="manual_style" style="hyphens: manual">dictionary</div>
	<div lang="en-US" class="probe" id="disabled" style="hyphenate-limit-chars: 20">dictionary</div>
	<div lang="en-US" class="probe" id="line_limit" style="hyphenate-limit-lines: 0">dictionary</div>
	<div lang="en-US" class="probe" id="nowrap" style="white-space: nowrap">dictionary</div>
	<div class="probe" id="inline_word" lang="en-US"><a id="dictionary_link" href="#american">dictio</a><span id="dictionary_tail">nary</span></div>
	<div class="probe" id="german" lang="de-DE">Donaudampfschifffahrt</div>
	<div class="probe" id="french" lang="fr-CA">électricité</div>
	<div class="probe" id="decomposed_french" lang="fr-CA">e&#769;lectricite&#769;</div>
	<div class="probe" id="spanish" lang="es-MX">electricidad</div>
	<div class="probe" id="galician" lang="gl">galeguización</div>
	<div class="probe" id="italian" lang="it">interessante</div>
	<div lang="en-US" class="probe">represe<span id="preferred_mark">&shy;</span>ntation</div>
	<div lang="en-US" class="probe" id="needs_fallback">rep<span id="fallback_mark">&shy;</span>resentation</div>
	<div id="fallback_manual" class="probe" style="hyphens: manual">rep&shy;resentation</div>
	<div lang="en-US" class="grid" style="hyphens: auto"><span id="auto_min">dictionary</span><i></i></div>
	<div lang="en-US" class="grid"><span id="manual_min">dictionary</span><i></i></div>
	<div lang="en-US" class="probe" style="width: 13ch; hyphenate-limit-last: always">aaaaaa <a id="whole_word" href="#american">dictionary</a></div>
	"""
	if layout.parse(source) != OK or layout.layout(Vector2i(260, 1000)) != OK:
		_check(false, "Prepare automatic hyphenation: " + layout.get_render_error())
		return
	var plain: String = layout.get_plain_text()
	var american: int = plain.find("dictionary")
	var british: int = plain.find("dictionary", american + 10)
	var us_letter: Array[Rect2] = layout.get_selection_rects(american + 6, american + 7)
	var gb_letter: Array[Rect2] = layout.get_selection_rects(british + 6, british + 7)
	_check(american == 0 and british == 11 and us_letter.size() == 1 and gb_letter.size() == 1 and
		us_letter[0].position.y > _rect(layout, "american").position.y + 20.0 and
		gb_letter[0].position.y < _rect(layout, "british").position.y + 20.0,
		"US and British dictionaries produce different cuts without inserting characters into selectable text")
	for id: String in ["unknown_language", "other_script", "manual_style", "disabled", "line_limit", "nowrap"]:
		_check(is_equal_approx(_rect(layout, id).size.y, 24.0), id + " retains a single line")
	for id: String in ["german", "french", "spanish", "galician", "italian"]:
		_check(_rect(layout, id).size.y > 24.0, id + " uses its inherited language dictionary")
	_check(is_equal_approx(_rect(layout, "decomposed_french").size.y, _rect(layout, "french").size.y) and
		"e\u0301lectricite\u0301" in plain, "Canonical equivalents share dictionary points while keeping source text")
	var link: Rect2 = _rect(layout, "dictionary_link")
	var tail: Rect2 = _rect(layout, "dictionary_tail")
	_check(tail.position.y > link.position.y + 20.0 and
		layout.get_link_at(Vector2(link.end.x - 1.0, link.get_center().y)) == "#american",
		"A break at an inline boundary keeps the inserted glyph inside the preceding link")
	_check(_rect(layout, "preferred_mark").has_area() and _rect(layout, "fallback_mark").has_area(),
		"Authored breaks take precedence over dictionary points")
	_check(_rect(layout, "needs_fallback").size.y > _rect(layout, "fallback_manual").size.y,
		"Dictionary points are available inside an authored segment that is still too long")
	_check(_rect(layout, "auto_min").size.x < _rect(layout, "manual_min").size.x,
		"Automatically hyphenated words contribute their minimum line width to Grid")
	_check(layout.get_element_by_id("whole_word").get_client_rects().size() == 1,
		"The terminal-line limit moves the whole automatically hyphenated word")
	_check(layout.get_element_by_id("inline_word").set_attribute("lang", "und") == OK and layout.layout(Vector2i(260, 1000)) == OK and
		is_equal_approx(_rect(layout, "inline_word").size.y, 24.0), "Changing inherited language withdraws the previous points")

	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(180, 80)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(viewport.size)
	label.text = """<!doctype html><style>
		body { margin: 0; font: 20px/24px monospace; color: white; background: black; }
		div { width: 8ch; hyphens: auto; hyphenate-character: '='; }
		#ink { color: lime; }
	</style><div lang="en-US"><span id="ink">dictio</span>nary</div>"""
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	var ink: Rect2i = Rect2i(_label_rect(label, "ink"))
	var last_cell: int = roundi(float(ink.size.x) / 7.0)
	var painted: bool
	for y: int in range(ink.position.y, ink.end.y):
		for x: int in range(ink.end.x - last_cell, ink.end.x):
			var pixel: Color = image.get_pixel(x, y)
			painted = painted or (pixel.g > 0.3 and pixel.r < 0.05 and pixel.b < 0.05)
	_check(painted, "HTMLLabel paints the automatic replacement at the end of the first line")
	viewport.queue_free()


func _ink_center_x(image: Image, box: Rect2) -> float:
	var ink: float
	var moment: float
	for y: int in range(roundi(box.position.y), roundi(box.end.y)):
		for x: int in range(roundi(box.position.x), roundi(box.end.x)):
			var amount: float = image.get_pixel(x, y).r
			ink += amount
			moment += amount * float(x)
	return moment / ink if ink > 0.0 else 0.0


func _font_ink(image: Image, box: Rect2) -> float:
	var ink: float
	for y: int in range(roundi(box.position.y), roundi(box.end.y)):
		for x: int in range(roundi(box.position.x), roundi(box.end.x)):
			ink += image.get_pixel(x, y).r
	return ink


func _check_controls() -> void:
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(500, 350)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)

	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(250, 280)
	label.stylesheet = """
		.specific_pixel { background: red; }
		.tie_pixel { background: #35ae71; }
		.clip_literal { background-clip: content-box; }
	"""
	label.text = """
		<style>
		  body { margin: 0; }
		  @layer base, theme;
		  @layer theme { #layered_pixel { background: #2196f3; } }
		  @layer base { #reverted, #layered_pixel { background: red; } }
		  #specific_pixel { background: #17a2b8; }
		  .tie_pixel { background: red; }
		  #parent { color: red; background-color: color-mix(in srgb, currentColor, white); }
		  .bevel { display: inline-block; width: 28px; height: 25px; border: 8px solid #808080; }
		  .clip_literal, .clip_variable {
		    width: 18px; height: 10px; padding: 4px; border: 2px solid transparent;
		    background: #b53078 border-box;
		  }
		</style>
		<button id="reverted" type="button" style="background: var(--missing, revert)">Button</button>
		<button id="reference" type="button">Button</button>
		<div id="layered_pixel" style="width: 20px; height: 10px"></div>
		<div id="specific_pixel" class="specific_pixel" style="width: 20px; height: 10px"></div>
		<div id="tie_pixel" class="tie_pixel" style="width: 20px; height: 10px"></div>
		<div id="parent" style="width: 100px; height: 30px">
		  <div id="child" style="color: blue; background-color: inherit; width: 50px; height: 30px"></div>
		</div>
		<div><div id="groove" class="bevel" style="border-style: groove; border-radius: 12px"></div>
		  <div id="ridge" class="bevel" style="border-style: ridge"></div>
		  <div id="inset" class="bevel" style="border-style: inset"></div>
		  <div id="outset" class="bevel" style="border-style: outset"></div></div>
		<div id="clip_literal" class="clip_literal"></div>
		<div id="clip_variable" class="clip_variable"
		     style="--clip: content-box; background-clip: var(--clip); background-clip: bogus"></div>
		<div id="aspect_control" style="width: 60px; aspect-ratio: 3; background: #f34a80"></div>
	"""
	viewport.add_child(label)

	var markdown: MarkdownLabel = MarkdownLabel.new()
	markdown.position = Vector2(250, 0)
	markdown.size = Vector2(240, 240)
	var theme_font: SystemFont = SystemFont.new()
	theme_font.font_names = PackedStringArray(["Consolas"])
	markdown.add_theme_font_override("text_font", theme_font)
	markdown.add_theme_font_size_override("text_font_size", 28)
	var test_texture: ImageTexture = ImageTexture.create_from_image(Image.create_empty(1, 1, false, Image.FORMAT_RGBA8))
	for icon: StringName in [&"task_checked", &"task_unchecked", &"file_broken"]:
		markdown.add_theme_icon_override(icon, test_texture)
	markdown.parse_html = true
	markdown.text = """
<div>
<style>
@layer base, theme;
@layer theme { .themed { background: #edc231; } }
@layer base { #themed { background: red; font: 10px serif; } }
</style>
<div id="themed" class="themed" style="font: var(--missing, revert); width: 5ch; height: 1em">Theme</div>
<div style="width: 60px; aspect-ratio: 3; background: #f34a80"></div>
</div>
"""
	viewport.add_child(markdown)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	var reverted: Rect2 = _label_rect(label, "reverted")
	var reference: Rect2 = _label_rect(label, "reference")
	_check(reverted.has_area() and reverted.size.is_equal_approx(reference.size), "Button rollback restores default geometry")
	_check_pixel(image, reverted.position + Vector2(5.0, 5.0), Color("#f3f4f6"), "Button rollback restores the default fill")
	_check_pixel(image, reference.position + Vector2(5.0, 5.0), Color("#f3f4f6"), "Button default fill")
	_check_pixel(image, _label_rect(label, "layered_pixel").position + Vector2(5.0, 5.0),
		Color("#2196f3"), "HTMLLabel paints the highest priority layer")
	_check_pixel(image, _label_rect(label, "specific_pixel").position + Vector2(5.0, 5.0),
		Color("#17a2b8"), "HTMLLabel document ID rule wins over the additional class rule")
	_check_pixel(image, _label_rect(label, "tie_pixel").position + Vector2(5.0, 5.0),
		Color("#35ae71"), "HTMLLabel additional class rule wins a source-order tie")
	var parent: Rect2 = _label_rect(label, "parent")
	_check_pixel(image, parent.position + Vector2(15.0, 15.0), Color("#8080ff"), "Inherited mix uses the child's currentColor")
	_check_pixel(image, parent.position + Vector2(75.0, 15.0), Color("#ff8080"), "Parent mix uses its own currentColor")
	var dark: Color = Color("#535353")
	var light: Color = Color("#acacac")
	var groove: Vector2 = _label_rect(label, "groove").position
	var ridge: Vector2 = _label_rect(label, "ridge").position
	var inset: Vector2 = _label_rect(label, "inset").position
	var outset: Vector2 = _label_rect(label, "outset").position
	_check_pixel(image, groove + Vector2(20.0, 2.0), dark, "Rounded groove outer band")
	_check_pixel(image, groove + Vector2(20.0, 6.0), light, "Rounded groove inner band")
	_check_pixel(image, ridge + Vector2(20.0, 2.0), light, "Ridge outer band")
	_check_pixel(image, ridge + Vector2(20.0, 6.0), dark, "Ridge inner band")
	_check_pixel(image, inset + Vector2(20.0, 2.0), dark, "Inset top edge")
	_check_pixel(image, inset + Vector2(41.0, 20.0), light, "Inset right edge")
	_check_pixel(image, outset + Vector2(20.0, 2.0), light, "Outset top edge")
	_check_pixel(image, outset + Vector2(41.0, 20.0), dark, "Outset right edge")
	for id: String in ["clip_literal", "clip_variable"]:
		var border_box: Rect2i = Rect2i(_label_rect(label, id))
		var expected: Rect2i = Rect2i(border_box.position + Vector2i(6, 6), Vector2i(18, 10))
		var painted: Rect2i = _color_bounds(image, border_box, Color("#b53078"))
		_check(painted == expected, "%s longhand clips the shorthand background to the content box, got %s" % [id, painted])
	var theme_box: Rect2i = _color_bounds(image, Rect2i(250, 0, 240, 200), Color("#edc231"))
	var theme_width: int = roundi(5.0 * theme_font.get_string_size("0", HORIZONTAL_ALIGNMENT_LEFT, -1.0, 28).x)
	_check(theme_box.size == Vector2i(theme_width, 28), "Markdown HTML rollback restores the theme font, box=%s" % theme_box)
	var ratio_box: Rect2 = _label_rect(label, "aspect_control")
	_check(ratio_box.size.is_equal_approx(Vector2(60.0, 20.0)), "HTMLLabel uses the preferred aspect ratio")
	_check(_color_bounds(image, Rect2i(250, 0, 240, 200), Color("#f34a80")).size == Vector2i(60, 20),
		"Markdown HTML paints the box at the preferred aspect ratio")
	viewport.queue_free()


func _check_author_hover() -> void:
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(240, 160)
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(240.0, 160.0)
	label.text = """<style>
		body { margin: 0; }
		.card { display: block; width: 80px; height: 40px; }
	</style><div id="hover_card" class="card"></div><div style="height: 200px"></div>
	<div id="hover_second" class="card"></div><div style="height: 160px"></div>"""
	label.stylesheet = ".card:hover { width: 110px; } #hover_card:is(:hover) { width: 130px; }"
	viewport.add_child(label)
	await process_frame
	await process_frame
	var card: HTMLElement = label.get_element_by_id("hover_card")
	_check(is_equal_approx(card.get_bounding_rect().size.x, 80.0), "The document rule applies without hover")
	var motion: InputEventMouseMotion = InputEventMouseMotion.new()
	motion.position = Vector2(15.0, 15.0)
	motion.global_position = motion.position
	viewport.push_input(motion, true)
	await process_frame
	await process_frame
	await process_frame
	_check(card.matches(":hover"), "The control updates the real element's hover state")
	_check(is_equal_approx(card.get_bounding_rect().size.x, 130.0), "Additional rules trigger hover restyling and share specificity")
	label.set_scroll_offset(Vector2i(0, 240))
	await process_frame
	await process_frame
	await process_frame
	var second: HTMLElement = label.get_element_by_id("hover_second")
	_check(not card.matches(":hover") and second.matches(":hover"), "Scrolling updates the hover match under a stationary pointer")
	_check(is_equal_approx(second.get_bounding_rect().size.x, 110.0), "Scrolling activates the newly hovered rule")
	viewport.queue_free()


func _check_focus_within() -> void:
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(240, 160)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(240, 160)
	label.text = """<style>
	body { margin: 0; }
	.panel { width: 80px; }
	.panel:focus-within { width: 130px; }
	.panel:has(> button:focus)::before { content: attr(data-focus-label); }
	</style>
	<div class="panel" id="first_panel" data-focus-label="FIRST-FOCUSED"><button type="button" id="first_button">First</button></div>
	<div class="panel" id="second_panel" data-focus-label="SECOND-FOCUSED"><button type="button" id="second_button">Second</button></div>"""
	viewport.add_child(label)
	await process_frame
	await process_frame
	_check(label.query_selector(".panel:focus-within") == null, "No HTML element is focused initially")
	label.select_all()
	_check("FOCUSED" not in label.get_selected_text(), "The relational pseudo-element starts inactive")
	label.grab_focus()
	var tab: InputEventKey = InputEventKey.new()
	tab.keycode = KEY_TAB
	tab.pressed = true
	viewport.push_input(tab)
	await process_frame
	await process_frame
	var first: HTMLElement = label.get_element_by_id("first_panel")
	var second: HTMLElement = label.get_element_by_id("second_panel")
	var button: HTMLElement = label.get_element_by_id("first_button")
	_check(button.matches(":focus:focus-within"), "The focused button itself matches :focus-within")
	_check(button.matches(":enabled:not(:disabled)"), "An enabled button exposes its interaction state")
	_check(first.matches(":focus-within") and not second.matches(":focus-within"), "Only ancestors of the focused button match")
	_check(is_equal_approx(first.get_bounding_rect().size.x, 130.0), "Focus-within restyles the matching ancestor")
	label.select_all()
	_check("FIRST-FOCUSED" in label.get_selected_text(), "Focus activates a relational generated-content rule")
	var space: InputEventKey = InputEventKey.new()
	space.keycode = KEY_SPACE
	space.pressed = true
	viewport.push_input(space)
	await process_frame
	_check(button.matches(":active") and first.matches(":has(> button:active)"), "Pressing Space activates the focused button")
	space.pressed = false
	viewport.push_input(space)
	await process_frame
	_check(not button.matches(":active"), "Releasing Space clears the active match")
	viewport.push_input(tab)
	await process_frame
	await process_frame
	_check(not first.matches(":focus-within") and second.matches(":focus-within"), "Tab updates focus ancestry")
	_check(is_equal_approx(first.get_bounding_rect().size.x, 80.0) and is_equal_approx(second.get_bounding_rect().size.x, 130.0),
		"Moving focus refreshes both ancestor styles")
	label.select_all()
	_check("FIRST-FOCUSED" not in label.get_selected_text() and "SECOND-FOCUSED" in label.get_selected_text(),
		"Moving focus replaces the generated content")
	label.release_focus()
	await process_frame
	await process_frame
	_check(label.query_selector(".panel:focus-within") == null, "Releasing focus clears the match")
	label.select_all()
	_check("FOCUSED" not in label.get_selected_text(), "Releasing focus removes the relational generated content")
	_check(button.set_attribute("disabled", "") == OK, "Disable the first button")
	await process_frame
	await process_frame
	_check(button.matches(":disabled:not(:enabled)"), "Disabling a button updates selector state")
	viewport.queue_free()


func _check_text_emphasis() -> void:
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(490, 320)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(490, 320)
	label.css_diagnostics_enabled = true
	label.text = """<style>
	body { margin: 0; font-family: 'Consolas', monospace; font-size: 30px; background: black; color: white; }
	.row { width: 400px; height: 54px; }
	#over { --mark: "\\25A0"; text-emphasis: var(--mark) #ff0000; text-emphasis-position: over; }
	#under { text-emphasis: "#ignored" #00ff00; text-emphasis-position: left under; }
	#under > span { text-emphasis-color: #00ffff; text-shadow: 12px 0 #8800ff; }
	#none { text-emphasis: red; }
	#reset > span { color: #ffff00; }
	#reset { text-emphasis: filled dot magenta; }
	#reset > span { text-emphasis: filled dot; }
	#wrap { width: 95px; line-height: 18px; text-emphasis: open circle #f8a600; }
	#wrap > span { text-emphasis-position: under; }
	#clip { position: absolute; left: 415px; top: 12px; width: 51px; height: 27px; border-radius: 9px;
		overflow: hidden; text-emphasis: "X" #0066ff; }
	</style>
	<div id="over" class="row">Á A, #</div>
	<div id="under" class="row"><span>A</span></div>
	<div id="none" class="row">ABC</div>
	<div id="reset" class="row"><span>A</span></div>
	<div id="wrap">A A <span>A A</span> A A</div>
	<div id="clip">AAA</div>"""
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	var over: Rect2i = Rect2i(_label_rect(label, "over"))
	var under: Rect2i = Rect2i(_label_rect(label, "under"))
	var reset: Rect2i = Rect2i(_label_rect(label, "reset"))
	var none: Rect2i = Rect2i(_label_rect(label, "none"))
	var clip: Rect2i = Rect2i(_label_rect(label, "clip"))
	var marks: PackedInt32Array = _colored_column_centers(image, over, Color.RED)
	_check(marks.size() == 3, "Emphasis paints three marks for accents, spaces and punctuation, got %s" % [marks])
	if marks.size() == 3:
		_check(marks[1] - marks[0] > 25 and marks[2] - marks[1] > 38, "Punctuation and spaces do not get a mark")
	var upper_mark: Rect2i = _color_bounds(image, over, Color.RED)
	var upper_base: Rect2i = _color_bounds(image, over, Color.WHITE)
	_check(upper_mark.has_area() and upper_base.has_area() and upper_mark.end.y <= upper_base.position.y, "Over marks paint above text")
	var lower_mark: Rect2i = _color_bounds(image, under, Color.CYAN)
	var lower_base: Rect2i = _color_bounds(image, under, Color.WHITE)
	_check(lower_mark.has_area() and lower_base.has_area() and lower_mark.position.y >= lower_base.end.y,
		"Under marks paint below text and inherit the emphasis string")
	_check(_color_bounds(image, Rect2i(under.position.x + 12, lower_mark.position.y, 12, lower_mark.size.y), Color("#8800ff")).has_area(),
		"Text shadow includes the emphasis glyph")
	_check(not _color_bounds(image, reset, Color.MAGENTA).has_area() and _color_bounds(image, reset, Color.YELLOW).has_area(),
		"Shorthand resets omitted emphasis color to currentColor")
	_check(_color_bounds(image, none, Color.RED).size == Vector2i.ZERO, "Shorthand resets omitted emphasis style")
	_check(_label_rect(label, "wrap").size.y > 60.0, "Wrapped lines reserve space for marks on both sides")
	_check(_color_bounds(image, clip, Color("#0066ff")).has_area() and not _color_bounds(image,
		Rect2i(clip.end.x, clip.position.y, 24, clip.size.y), Color("#0066ff")).has_area(), "Emphasis obeys overflow clipping")
	_check(label.get_css_diagnostics().is_empty(), "Supported emphasis leaves no diagnostics")

	var layout: HTMLLayout = HTMLLayout.new()
	_check(layout.parse('<p style="text-emphasis: &quot;*&quot; red">Alpha Beta</p>') == OK and layout.layout(Vector2i(200, 100)) == OK,
		"Lay out selectable text with emphasis")
	_check(layout.get_plain_text().strip_edges() == "Alpha Beta", "Emphasis marks do not enter copied text")
	_check(not layout.get_selection_rects(0, 5).is_empty(), "Base text keeps selection geometry")
	viewport.queue_free()


func _colored_column_centers(image: Image, region: Rect2i, expected: Color) -> PackedInt32Array:
	var positions: PackedInt32Array = PackedInt32Array()
	var start: int = -1
	for x: int in range(region.position.x, region.end.x + 1):
		var found: bool
		if x < region.end.x:
			for y: int in range(region.position.y, region.end.y):
				if _same_color(image.get_pixel(x, y), expected):
					found = true
					break
		if found and start < 0:
			start = x
		elif not found and start >= 0:
			positions.append(roundi(float(start + x - 1) / 2.0))
			start = -1
	return positions


func _check_underline_position() -> void:
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(460, 330)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(460, 330)
	label.css_diagnostics_enabled = true
	label.text = """<!doctype html><style>
	  body { margin: 0; font: 24px Consolas, monospace; background: black; color: white; }
	  .line { position: absolute; width: 65px; height: 57px; }
	  .line, .decoration { text-decoration: underline #ff2200 2px; }
	  #zero, #under_zero { text-underline-offset: 0; }
	  #positive { --offset: calc(4px + 0.25em); text-underline-offset: var(--offset); }
	  #negative { text-underline-offset: -4px; }
	  #auto, #font, #font_shift, #left, #invalid { text-underline-offset: auto; }
	  #font, #font_shift { text-underline-position: from-font; }
	  #font_shift { text-underline-offset: 0.25em; }
	  #left { text-underline-position: left; }
	  #under_zero, #under_shift { text-underline-position: under right; }
	  #under_shift { text-underline-offset: 25%; text-shadow: 0 5px cyan; }
	  #invalid { text-underline-position: left right; text-underline-offset: 2; }
	  #outer { text-underline-offset: 9px; }
	  #outer span { text-underline-offset: 0; }
	  #fraction { text-underline-offset: 25%; }
	  #length { text-underline-offset: 0.25em; }
	  .decorator { position: absolute; width: 65px; height: 62px; }
	  .decoration { font-size: 32px; }
	  #small { text-underline-offset: 0; }
	  #plain_over, #offset_over { text-decoration-line: overline; }
	  #offset_over { text-underline-offset: 12px; }
	</style>
	<div class="line" id="zero" style="left: 0; top: 0">gyp</div>
	<div class="line" id="positive" style="left: 75px; top: 0">gyp</div>
	<div class="line" id="negative" style="left: 150px; top: 0">gyp</div>
	<div class="line" id="auto" style="left: 225px; top: 0">gyp</div>
	<div class="line" id="font" style="left: 300px; top: 0">gyp</div>
	<div class="line" id="font_shift" style="left: 375px; top: 0">gyp</div>
	<div class="line" id="under_zero" style="left: 0; top: 60px">gyp</div>
	<div class="line" id="under_shift" style="left: 75px; top: 60px">gyp</div>
	<div class="line" id="left" style="left: 150px; top: 60px">gyp</div>
	<div class="line" id="invalid" style="left: 225px; top: 60px">gyp</div>
	<div class="line" id="outer" style="left: 300px; top: 60px">g<span>y</span>p</div>
	<div class="decorator" id="length" style="left: 0; top: 135px"><span class="decoration">gyp</span></div>
	<div class="decorator" id="fraction" style="left: 75px; top: 135px"><span class="decoration">gyp</span></div>
	<div class="decorator" id="small" style="left: 150px; top: 135px"><span class="decoration">gyp</span></div>
	<div class="line" id="plain_over" style="left: 0; top: 220px">gyp</div>
	<div class="line" id="offset_over" style="left: 75px; top: 220px">gyp</div>"""
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	var relative_y: Dictionary[String, int] = {}
	var rules: Dictionary[String, Rect2i] = {}
	for id: String in ["zero", "positive", "negative", "auto", "font", "font_shift", "under_zero", "under_shift",
		"left", "invalid", "outer", "length", "fraction", "small", "plain_over", "offset_over"]:
		var region: Rect2i = Rect2i(_label_rect(label, id))
		var rule: Rect2i = _color_bounds(image, region, Color("#ff2200"))
		_check(rule.has_area(), "Underline fixture paints %s" % id)
		relative_y[id] = rule.position.y - region.position.y
		rules[id] = rule
	_check(relative_y["positive"] - relative_y["zero"] == 10, "Computed offset is measured from the baseline")
	_check(relative_y["negative"] - relative_y["zero"] == -4, "Negative offset moves inward")
	_check(relative_y["font_shift"] - relative_y["font"] == 6, "from-font adds the offset to the font metric")
	_check(relative_y["under_zero"] > relative_y["zero"] and relative_y["under_shift"] - relative_y["under_zero"] == 6,
		"Under positions the line below the text and uses the author offset")
	_check(relative_y["auto"] == relative_y["left"] and relative_y["auto"] == relative_y["invalid"],
		"Left uses auto in horizontal writing and invalid declarations are rejected")
	_check(relative_y["length"] - relative_y["small"] == 6 and relative_y["fraction"] - relative_y["small"] == 8,
		"Inherited lengths stay absolute and percentages resolve on the decorating element")
	_check(relative_y["outer"] - relative_y["zero"] == 9 and rules["outer"].size.y == 2,
		"The author offset stays with the originating underline across inline children")
	var shifted_rule: Rect2i = rules["under_shift"]
	_check_pixel(image, Vector2(shifted_rule.get_center() + Vector2i(0, 5)), Color.CYAN, "Shadow follows the offset underline")
	_check(relative_y["plain_over"] == relative_y["offset_over"], "Underline offset has no effect on overline")
	_check(label.get_css_diagnostics().size() == 2, "Two invalid underline values produce diagnostics")
	viewport.queue_free()


func _check_color_spaces() -> void:
	# CSS Color 4 supplies reference values for the non-neutral XYZ, Lab, P3 and Rec. 2020 samples
	var swatches: Dictionary[String, String] = {
		"hwb(120 30% 50%)": "#4d804d",
		"hwb(45 40% 80%)": "#555555",
		"hwb(0.5turn 30 50)": "#4d8080",
		"lab(57.9% 11.4 -53.7)": "#6c88ea",
		"lch(57.9% 36.5978% 281.9854deg)": "#6c88ea",
		"oklab(65% 0 -37.5%)": "#6c88ea",
		"oklch(65% 0.15 270)": "#6c88ea",
		"color(display-p3 0.445 0.529 0.891)": "#6c88ea",
		"color(display-p3-linear 0.25 0.25 0.25)": "#898989",
		"color(rec2020 0.522 0.557 0.892)": "#6c88ea",
		"color(a98-rgb 50% 0.5 0.5)": "#818181",
		"color(prophoto-rgb 0.5 0.5 0.5)": "#929292",
		"color(xyz-d50 0.2004941 0.140872 0.4470838)": "#7654cd",
		"color(xyz 0.216595 0.1459994 0.594365)": "#7654cd",
		"color(srgb-linear 25% 0.25 0.25)": "#898989",
		"color(srgb none 60% 0.25 / 50%)": "#004c20",
		"color(srgb-linear none 0.25 0.25)": "#008989",
		"color-mix(in display-p3-linear, black, white)": "#bcbcbc",
		"color-mix(in lab, black, white)": "#777777",
		"color-mix(in hsl longer hue, red, blue)": "#00ff00",
		"color-mix(in hwb, white, lime)": "#80ff80",
		"color-mix(in srgb, color(srgb 1.5 0 0), black)": "#bf0000",
		"color-mix(in srgb, red, lime, blue)": "#555555",
		"color(srgb sqrt(0.25) calc(2 * 25%) abs(-50%))": "#808080",
		"rgb(calc(2 * 64) 50% 128)": "#808080",
		"rgba(calc(100%), min(0%, 10%), 0%, calc(1 / 2))": "#800000",
		"hsl(calc(1turn / 3), calc(2 * 50%), min(60%, 50%))": "#00ff00",
		"hwb(atan2(1, 0) min(20%, 40%) calc(5 * 2%) / calc(1 / 2))": "#467319",
		"color-mix(in srgb, red calc(25% + 25%), lime, blue)": "#804040",
		"color-mix(in srgb, red calc(-5%), lime calc(120%))": "#00ff00",
		"oklch(65% 0.15 calc(0.75turn))": "#6c88ea",
		"color-mix(in srgb, red 50%, lime, blue)": "#804040",
		"color-mix(in srgb, 100% red, lime 100%, blue)": "#808000",
		"color-mix(in srgb, red 20%, lime 20%, blue 20%)": "#333333",
		"color-mix(in srgb, red 0%, lime 0%, blue 0%)": "#000000",
		"color-mix(in srgb-linear, white 50%)": "#808080",
		"color-mix(in srgb, transparent, lime, currentColor)": "#00aa00",
		"color-mix(in hsl increasing hue, red, lime, blue)": "#00ff00",
		"color-mix(in srgb, color(srgb none 0.4 0 / none), color(srgb 0.8 0 0 / 0.5))": "#661a00",
		"color-mix(in srgb, color-mix(in display-p3-linear, color(display-p3-linear 2 0 0), black), currentColor)": "#8c6500",
	}
	var html: String = """<style>body { margin: 0; background: black; color: lime; }
		.swatch { position: absolute; width: 12px; height: 12px; }
		@supports (color: color(xyz-d65 0 0 0)) { #positive { background: lime; } }
		@supports (color: color(oklab 0.8 0 0)) { #invalid { background: red; } }
		</style>"""
	var points: Dictionary[String, Vector2] = {}
	var index: int
	for expression: String in swatches:
		var position: Vector2i = Vector2i((index % 8) * 15, floori(float(index) / 8.0) * 15)
		html += '<div class="swatch" style="left: %dpx; top: %dpx; --swatch: %s; background: var(--swatch)"></div>' % [
			position.x, position.y, expression,
		]
		points[expression] = Vector2(position + Vector2i(5, 5))
		index += 1
	html += """<div class="swatch" id="positive" style="left: 0; top: 75px"></div>
		<div class="swatch" id="invalid" style="left: 15px; top: 75px; background: lime; background: hwb(0, 0%, 0%)"></div>"""
	var viewport: SubViewport = SubViewport.new()
	viewport.size = Vector2i(140, 90)
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(viewport)
	var label: HTMLLabel = HTMLLabel.new()
	label.size = Vector2(140, 90)
	label.text = html
	viewport.add_child(label)
	await process_frame
	await process_frame
	await RenderingServer.frame_post_draw
	var image: Image = viewport.get_texture().get_image()
	for expression: String in swatches:
		_check_pixel(image, points[expression], Color(swatches[expression]), expression)
	_check_pixel(image, Vector2(5.0, 80.0), Color.LIME, "Feature query accepts a predefined color space")
	_check_pixel(image, Vector2(20.0, 80.0), Color.LIME, "Invalid color declaration preserves the earlier valid value")
	viewport.queue_free()


func _rect(layout: HTMLLayout, id: String) -> Rect2:
	var element: HTMLElement = layout.get_element_by_id(id)
	return element.get_bounding_rect() if element != null else Rect2()


func _label_rect(label: HTMLLabel, id: String) -> Rect2:
	var element: HTMLElement = label.get_element_by_id(id)
	return element.get_bounding_rect() if element != null else Rect2()


func _has_diagnostic(layout: HTMLLayout, property: String, value: String, source: String, path: String = "") -> bool:
	for entry: Dictionary in layout.get_css_diagnostics():
		if entry["kind"] == "unsupported_value" and entry["property"] == property and entry["value"] == value:
			if entry["source"] == source and entry["path"] == path:
				return true
	return false


func _check_pixel(image: Image, position: Vector2, expected: Color, description: String) -> void:
	var actual: Color = image.get_pixelv(Vector2i(position))
	_check(_same_color(actual, expected), "%s at %s is %s, expected %s" % [description, position, actual, expected])


func _color_bounds(image: Image, region: Rect2i, expected: Color) -> Rect2i:
	var bounds: Rect2i
	for y: int in range(region.position.y, region.end.y):
		for x: int in range(region.position.x, region.end.x):
			if _same_color(image.get_pixel(x, y), expected):
				var pixel: Rect2i = Rect2i(x, y, 1, 1)
				bounds = bounds.merge(pixel) if bounds.has_area() else pixel
	return bounds


func _same_color(first: Color, second: Color) -> bool:
	const TOLERANCE: float = 2.1 / 255.0
	return absf(first.r - second.r) <= TOLERANCE and absf(first.g - second.g) <= TOLERANCE and absf(first.b - second.b) <= TOLERANCE


func _check(condition: bool, description: String) -> void:
	if not condition:
		_failures += 1
		push_error(description)
