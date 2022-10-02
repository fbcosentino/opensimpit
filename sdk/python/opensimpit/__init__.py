# Main file (including description and license) is osp.py

# Below is a workaround to avoid requiring users to pip install modules
# as they are bundled within this one. 
# OpenSimPit module must have zero user-installed external dependencies.
import os, sys
_this_path = str(os.path.dirname(__file__))
if (_this_path not in sys.path):
    sys.path.append(_this_path)

import osp as osplib
from osp import OpenSimPit