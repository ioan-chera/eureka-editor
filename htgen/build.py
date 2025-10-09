import os
import sys
from bs4 import BeautifulSoup
import shutil
import errno
import markdown

cur_path = os.path.dirname(os.path.realpath(__file__))
out_path = os.path.join(cur_path, '..', 'htdocs-OUTPUT')
os.makedirs(out_path, exist_ok=True)

legacy_content = (
    'Docs_CommandList.html',
    'Docs_Index.html',
    'Docs_Invoking.html',
    'Docs_Keys.html',
    'Docs_KeySystem.html',
    'index.html',
    'Main_About.html',
    'Main_Changes072.html',
    'Main_Changes074.html',
    'Main_Changes081.html',
    'Main_Changes084.html',
    'Main_Changes088.html',
    'Main_Changes095.html',
    'Main_Changes100.html',
    'Main_Changes107.html',
    'Main_Changes111.html',
    'Main_Changes121.html',
    'Main_Changes124.html',
    'Main_Changes127.html',
    'Main_Changes2.0.0.html',
    'Main_Changes2.0.1.html',
    'Main_Changes2.0.2.html',
    'Main_Changes2.1.0.html',
    'Main_Credits.html',
    'Main_Download.html',
    'Main_History.html',
    'Main_SVN_Logs.html',
    'VisExp_Main.html',
)

static_extra = (
    'shots', 'custom_logo.png', 'eureka.css', 'grid.png'
)

template = 'template.html'

with open(os.path.join(cur_path, template)) as f:
    template_data = f.read().replace('&nbsp;', ' ')

template_soup = BeautifulSoup(template_data, 'html.parser')

for item in legacy_content:
    path = os.path.join(cur_path, item)
    with open(os.path.join(cur_path, path)) as f:
        item_data = f.read().replace('&nbsp;', ' ')
    item_soup = BeautifulSoup(item_data, 'html.parser')

    if item == 'Main_Credits.html':
        with open(os.path.join(cur_path, '..', 'AUTHORS.md')) as f:
            authors_content = f.read()
        item_soup.div.append(BeautifulSoup(markdown.markdown(authors_content, extensions=['fenced_code']), 'html.parser'))
    elif item.startswith('Main_Changes') and item.endswith('.html'):
        # Extract version from filename (e.g., 'Main_Changes2.0.0.html' -> '2.0.0')
        version = item[len('Main_Changes'):-len('.html')]
        changelog_path = os.path.join(cur_path, '..', 'changelogs', f'{version}.md')
        with open(changelog_path) as f:
            changes_content = f.read()
        item_soup.div.append(BeautifulSoup(markdown.markdown(changes_content, extensions=['fenced_code']), 'html.parser'))

    template_soup.find(id='wikitext').replaceWith(item_soup)

    try:
        title = os.path.splitext(item)[0].split('_')[1]
    except:
        title = 'MainPage'  # index.html only one

    with open(os.path.join(out_path, item), 'w') as f:
        f.write(str(template_soup).replace('$(TITLE)', title))


def copyanything(src, dst):
    try:
        if os.path.exists(dst):
            shutil.rmtree(dst)
        shutil.copytree(src, dst)
    except OSError as exc: # python >2.5
        if exc.errno in (errno.ENOTDIR, errno.EINVAL):
            shutil.copy(src, dst)
        else: raise

for extra in static_extra:
    copyanything(os.path.join(cur_path, extra), os.path.join(out_path, extra))

