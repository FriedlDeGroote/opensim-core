#!/usr/bin/env python

import os

from setuptools import setup

# This provides the variable `__version__`.
execfile('opensim/version.py')

setup(name='opensim',
      version=__version__,
      description='OpenSim Simulation Framework',
      author='OpenSim Team',
      author_email='ahabib@stanford.edu',
      url='http://opensim.stanford.edu/',
      license='Apache 2.0',
      packages=['opensim'],
      package_data={'opensim': ['_*.*']},
      include_package_data=True,
      classifiers=[
          'Intended Audience :: Science/Research',
          'Operating System :: OS Independent',
          'Programming Language :: Python :: 2.7',
          'Topic :: Scientific/Engineering :: Physics',
          ],
      )
