import os
import subprocess

script_path = os.path.dirname(os.path.realpath(__file__))
out_path = os.path.join(script_path, '..', 'out')
os.chdir(out_path)
subprocess.call('Debug/poker-tests.exe')
