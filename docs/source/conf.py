#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import sys
import glob
import sphinx_rtd_theme
from geopmdpy import __version__ as version

linkcheck_timeout = 1

_docs_dir = os.path.dirname(__file__)

# Repo dir if this is for the base build
_repo_dir = os.path.abspath(os.path.join(_docs_dir, '..', '..'))
if os.path.exists(os.path.join(_repo_dir, 'geopmdpy')):
    # Repo dir if this is the service build
    _repo_dir = os.path.abspath(os.path.join(_docs_dir, '..', '..', '..'))
    _geopmdpy_dir = os.path.join(_repo_dir, 'service')
    sys.path.insert(0, _geopmdpy_dir)
_ext_dir = os.path.join(_docs_dir, '_ext')
_geopmpy_dir = os.path.join(_repo_dir, 'scripts')
sys.path.insert(0, _geopmpy_dir)
sys.path.append(_ext_dir)

# -- Project information -----------------------------------------------------

project = 'GEOPM'
copyright = '2015 - 2024 Intel Corporation. All rights reserved.'

author = 'Intel (R) Corporation'

# -- Helper functions --------------------------------------------------------

# gets the description of the man page from the rst file
def get_description(rst_file_name):
    previous_line = ""
    current_line = ""
    with open(rst_file_name, 'r') as rst_file:
        # loop through each line of text
        for line in rst_file:
            line = line.rstrip()
            previous_line = current_line
            current_line = line
            # if you find the starting header
            if (current_line and current_line == len(current_line) * '='):
                rst_file.close()
                return previous_line.split(" -- ")[1]
    # if you get to the end of the file without finding the starting header
    rst_file.close()
    return rst_file_name.split('.')[0].replace('_', ' ')

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
master_doc = 'index'

extensions = [
    'sphinx.ext.napoleon',
    'sphinx_rtd_theme',
    'sphinx.ext.autosectionlabel',
    'sphinxemoji.sphinxemoji',
    'sphinx.ext.intersphinx',
    'sphinx.ext.todo',
    'geopmlint',
    'geopm_rst_extensions',
    'sphinx_tabs.tabs',
]

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
#
# source_suffix = ['.rst', '.md']
source_suffix = '.rst'

napoleon_google_docstring = True

autodoc_mock_imports = ['geopmdpy.gffi',
                        'geopmpy.gffi',
                        'pandas',
                        'numpy',
                        'natsort',
                        'dasbus',
                        'psutil']

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

autosectionlabel_prefix_document = True

# Fail the documentation build for nitpicky things like broken cross-references.
nitpicky = True

# A boolean that decides whether parentheses are appended to function and method role text
# (e.g. the content of :func:`input`) to signify that the name is callable.
# add_function_parentheses set to True causes Sphinx to automatically add trailing parentheses
# to enhance readability to the end of Python function names in the :py:func:
# which is a python reference link to the documentation.
add_function_parentheses = True

intersphinx_mapping = {
    'python': ('https://docs.python.org/3', None),
    'dasbus': ('https://dasbus.readthedocs.io/en/stable', None),
    'pygobject': ('https://pygobject.readthedocs.io/en/latest', None),
    'cffi': ('https://cffi.readthedocs.io/en/latest', None),
    'pandas': ('https://pandas.pydata.org/docs/', None),
    'psutil': ('https://psutil.readthedocs.io/en/latest', None),
}

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#

html_theme = 'sphinx_rtd_theme'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_logo = 'https://geopm.github.io/images/geopm-logo-clear.png'

# -- Options for manual page output -------------------------------------------------

rst_files = [
    "geopm::Agent.3",
    "geopm::Agg.3",
    "geopm::CircularBuffer.3",
    "geopm::CNLIOGroup.3",
    "geopm::Comm.3",
    "geopm::CPUActivityAgent.3",
    "geopm::CpuinfoIOGroup.3",
    "geopm::Daemon.3",
    "geopm::Endpoint.3",
    "geopm::Exception.3",
    "geopm::GPUActivityAgent.3",
    "geopm::Helper.3",
    "geopm::IOGroup.3",
    "geopm::MonitorAgent.3",
    "geopm::MPIComm.3",
    "geopm::MSRIO.3",
    "geopm::MSRIOGroup.3",
    "geopm::PlatformIO.3",
    "geopm::PlatformTopo.3",
    "geopm::PluginFactory.3",
    "geopm::PowerBalancer.3",
    "geopm::PowerBalancerAgent.3",
    "geopm::PowerGovernor.3",
    "geopm::PowerGovernorAgent.3",
    "geopm::ProfileIOGroup.3",
    "geopm::SampleAggregator.3",
    "geopm::SharedMemory.3",
    "geopm::TimeIOGroup.3",
    "geopm.7",
    "geopmaccess.1",
    "geopmadmin.1",
    "geopmagent.1",
    "geopm_agent.3",
    "geopm_agent_cpu_activity.7",
    "geopm_agent_ffnet.7",
    "geopm_agent_frequency_map.7",
    "geopm_agent_gpu_activity.7",
    "geopm_agent_monitor.7",
    "geopm_agent_power_balancer.7",
    "geopm_agent_power_governor.7",
    "geopmbench.1",
    "geopmctl.1",
    "geopm_ctl.3",
    "geopmdpy.7",
    "geopm_daemon.3",
    "geopmendpoint.1",
    "geopm_endpoint.3",
    "geopm_error.3",
    "geopmexporter.1",
    "geopm_field.3",
    "geopm_fortran.3",
    "geopm_hash.3",
    "geopm_imbalancer.3",
    "geopmlaunch.1",
    "geopm_pio.3",
    "geopm_pio.7",
    "geopm_pio_const_config.7",
    "geopm_pio_cnl.7",
    "geopm_pio_cpuinfo.7",
    "geopm_pio_dcgm.7",
    "geopm_pio_levelzero.7",
    "geopm_pio_nvml.7",
    "geopm_pio_profile.7",
    "geopm_pio_service.7",
    "geopm_pio_sst.7",
    "geopm_pio_time.7",
    "geopm_pio_msr.7",
    "geopm_pio_sysfs.7",
    "geopm_policystore.3",
    "geopm_prof.3",
    "geopmpy.7",
    "geopmread.1",
    "geopm_report.7",
    "geopm_sched.3",
    "geopmsession.1",
    "geopm_time.3",
    "geopm_topo.3",
    "geopm_version.3",
    "geopmwrite.1",
]

authors = ["Christopher Cantalupo", "Brad Geltz", "Konstantin Rebrov"]
# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = []
for rst_file in rst_files:
    the_tuple = (
        rst_file,                           # startdocname
        rst_file.split('.')[0],             # name
        get_description(rst_file + ".rst"), # description
        authors,                            # authors
        int(rst_file.split('.')[1])         # section
    )
    man_pages.append(the_tuple)
