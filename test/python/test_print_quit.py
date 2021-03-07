import subprocess
import argparse

# test script expects the executable as argument
parser = argparse.ArgumentParser()
parser.add_argument('--executable',
                    help='full path to executable')
parser.add_argument('--version',
                    help='Eureka version string')
args = parser.parse_args()

IMMEDIATE_RUN_TIMEOUT = 3

def test_help_command():
    result = str(subprocess.check_output([args.executable, '--help'],
                                         timeout=IMMEDIATE_RUN_TIMEOUT))
    assert 'Eureka is free software, under the terms of the GNU General' in result
    assert 'USAGE: ' in result


def test_version_command():
    result = str(subprocess.check_output([args.executable, '--version'],
                                         timeout=IMMEDIATE_RUN_TIMEOUT))
    assert args.version in result


test_help_command()
test_version_command()
