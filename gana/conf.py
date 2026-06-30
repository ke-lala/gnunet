# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))


# -- Project information -----------------------------------------------------
import sys
import os

project = 'GANA'
copyright = '2024, GNUnet Project'
author = 'GNUnet Project'

sys.path.append(os.path.abspath("_exts"))

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
  'httpdomain.httpdomain',
  'typescriptdomain',
  'sphinx.ext.todo',
  #'sphinx_book_theme',
  #'breathe'
]

#breathe_projects = {
#    "gnunet": "../gnunet/doc/doxygen/xml/",
#}

#breathe_default_project = "gnunet"

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'

html_sidebars = {
    #"**": ["navbar-logo.html", "sbt-sidebar-nav.html", "search-field.html"]
    "**": ["search-field.html", "sbt-sidebar-nav.html"]
}

html_theme_options = {
    #'logo_only': True,
    #'display_version': False,
    #'prev_next_buttons_location': 'bottom',
    #'style_external_links': False,
    #'vcs_pageview_mode': '',
    #'style_nav_header_background': 'transparent', # Possibly problematic with our CSP
    # Toc options
    #'collapse_navigation': True,
    #'sticky_navigation': True,
    #'navigation_depth': 4,
    #'includehidden': True,
    #'titles_only': False
    #"navbar_start": ["navbar-logo"],
    #"header_links_before_dropdown": 8,
    #"article_header_start": ["breadcrumbs.html"],
    #"navbar_center": ["navbar-nav"],
    #"navbar_end": [],
    #"navbar_persistent": [],
}

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

# These paths are either relative to html_static_path
# or fully qualified paths (eg. https://...)
#html_css_files = [
#    'css/custom.css',
#]

html_logo = "images/gnunet-blue.png"

# Display to-do items in docs
todo_include_todos = True

primary_domain = "c"

highlight_language = "c"

rst_prolog = f"""
.. role:: c(code)
   :language: c

.. role:: bolditalic
   :class: bolditalic
"""

rst_epilog = """
"""


