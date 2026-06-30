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

project = 'GNUnet'
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
]

if os.environ.get("SPHINX_MULTIVERSION"):
    extensions.append('sphinx_multiversion')

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
    "**": ["globaltoc.html", "versioning.html"]
}

html_theme_options = {
    #'logo_only': True,
    'display_version': True,
    'collapse_navigation': True,
}

man_make_section_directory = False
man_pages = [
        ('about', 'gnunet', 'About GNUnet', '', '1'),
        ('installing', 'gnunet', 'Installing GNUnet', '', '2'),
        ('users/start', 'gnunet', 'Starting and stopping', '', '3'),
        ('users/gns', 'gnunet', 'The GNU Name System', '', '4'),
        ('users/reclaim', 'gnunet', 're:claimID', '', '5'),
        ('users/fs', 'gnunet', 'GNUnet File-Sharing', '', '6'),
        ('users/vpn', 'gnunet', 'GNUnet VPN', '', '7'),
        ('users/messenger', 'gnunet', 'GNUnet Messenger', '', '8'),
        ('users/configuration', 'gnunet', 'GNUnet advanced configuration', '', '9'),
        ('developers/contributing', 'gnunet-dev', 'Contributing', '', '1'),
        ('developers/style', 'gnunet-dev', 'Style and workflow', '', '2'),
        ('developers/util', 'gnunet-dev', 'libgnunetutil', '', '3'),
        ('developers/rest/rest', 'gnunet-dev', 'REST API', '', '5'),
        ('developers/tutorial', 'gnunet-dev', 'GNUnet Developer Tutorial', '', '6'),
        ('livingstandards', 'lsd', 'Living Standards', '', '1'),
]

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

# These paths are either relative to html_static_path
# or fully qualified paths (eg. https://...)
html_css_files = [
    'css/custom.css',
]

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

if os.environ.get("GNUNET_VERSION"):
    rst_prolog = ".. |gnunet_version| replace:: " + os.environ["GNUNET_VERSION"]
else:
    rst_prolog = ".. |gnunet_version| replace:: latest"

# Whitelist pattern for tags (set to None to ignore all tags)
#smv_tag_whitelist = r'^.*$'
smv_tag_whitelist = 'latest'

# Whitelist pattern for branches (set to None to ignore all branches)
smv_branch_whitelist = r'^(v\d+\.\d+\.x|master)$'
#smv_branch_whitelist = 'master'

# Whitelist pattern for remotes (set to None to use local branches only)
smv_remote_whitelist = None

# Pattern for released versions
smv_released_pattern = r'^refs/(tags/latest|heads/(?!master)).*$'
#smv_released_pattern = r'^refs/(heads/[^/]+)/(?!master).*$'

# Format for versioned output directories inside the build directory
smv_outputdir_format = '{ref.name}'

# Determines whether remote or local git branches/tags are preferred if their output dirs conflict
smv_prefer_remote_refs = False
