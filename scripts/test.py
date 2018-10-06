import os

script_path = os.path.dirname(os.path.realpath(__file__))
out_path = os.path.join(script_path, '..', 'out')
os.chdir(out_path)
os.system('ctest -C Debug --output-on-failure')
