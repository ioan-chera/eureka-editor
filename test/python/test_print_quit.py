import argparse
import re
import subprocess

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

    parms = set()

    # Check that all the <name> columns are aligned
    saved_pos = None
    for line in lines:
        pos = line.find('<')
        match = re.search('--[a-z_]+', line)
        if match:
            parm = match.group()
            parms.add(parm)
        if pos != -1:
            if saved_pos is not None:
                assert pos == saved_pos
            else:
                saved_pos = pos

    assert parms == {'--home', '--install', '--log', '--config', '--help', '--version', '--debug',
        '--quiet', '--file', '--merge', '--iwad', '--port', '--warp',
    }

    # Check that '<' marked arguments (like -warp) have an extra newline after
    in_warp = False
    for line in lines:
        if '-warp' in line:
            in_warp = True
            continue
        if in_warp:
            assert not line.strip() # check that we have an empty line here
            in_warp = False

def test_version_command():
    result = str(subprocess.check_output([args.executable, '--version'],
                                         timeout=IMMEDIATE_RUN_TIMEOUT))
    assert args.version in result


test_help_command()
test_version_command()
