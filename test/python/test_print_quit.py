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
    result = subprocess.check_output([args.executable, '--help'],
                                      timeout=IMMEDIATE_RUN_TIMEOUT).decode('utf-8')
    assert 'Eureka is free software, under the terms of the GNU General' in result
    assert 'USAGE: ' in result
    assert '--version' in result
    lines = result.split('\n')

    # Check that all the <name> columns are aligned
    saved_pos = None
    for line in lines:
        pos = line.find('<')
        if pos != -1:
            if saved_pos is not None:
                assert pos == saved_pos
            else:
                saved_pos = pos


def test_version_command():
    result = str(subprocess.check_output([args.executable, '--version'],
                                         timeout=IMMEDIATE_RUN_TIMEOUT))
    assert args.version in result


test_help_command()
test_version_command()
