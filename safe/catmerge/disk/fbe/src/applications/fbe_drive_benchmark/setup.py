from distutils.core import setup
import py2exe

options = {"py2exe": {"bundle_files": 1, "compressed": True }}
setup(console=['fbe_drive_benchmark.py'],
      options=options,
      zipfile=None)
