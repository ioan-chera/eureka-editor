import os
import sys
from bs4 import BeautifulSoup
import shutil
import errno

cur_path = os.path.dirname(os.path.realpath(__file__))
out_path = os.path.join(cur_path, '..', 'htdocs-OUTPUT')
os.makedirs(out_path, exist_ok=True)

legacy_content = (
    'Cookbook_Altar.html',
    'Cookbook_CurvedStairs.html',
    'Cookbook_Doors.html',
    'Cookbook_Lifts.html',
    'Cookbook_Prefabs.html',
    'Cookbook_Sky.html',
    'Cookbook_Stairs.html',
    'Cookbook_Teleporters.html',
    'Cookbook_ToxicPool.html',
    'Cookbook_Traps.html',
    'Docs_CommandList.html',
    'Docs_EditModes.html',
    'Docs_FAQ.html',
    'Docs_Index.html',
    'Docs_Invoking.html',
    'Docs_Keys.html',
    'Docs_KeySystem.html',
    'Docs_UghRef.html',
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
    'Main_Configuration.html',
    'Main_Credits.html',
    'Main_Download.html',
    'Main_History.html',
    'Main_SVN_Logs.html',
    'Main_TODO.html',
    'User_Basics.html',
    'User_Concepts.html',
    'User_Index.html',
    'User_Installation.html',
    'User_Intro.html',
    'User_Projects.html',
    'User_UI.html',
    'VisExp_Main.html',
)

static_extra = (
    'shots', 'user', 'custom_logo.png', 'eureka.css', 'grid.png'
)

template = 'template.html'

with open(os.path.join(cur_path, template)) as f:
    template_data = f.read()

template_soup = BeautifulSoup(template_data, 'html.parser')

for item in legacy_content:
    path = os.path.join(cur_path, item)
    with open(os.path.join(cur_path, path)) as f:
        item_data = f.read()
    item_soup = BeautifulSoup(item_data, 'html.parser')

    template_soup.find(id='wikitext').replaceWith(item_soup)

    try:
        title = os.path.splitext(item)[0].split('_')[1]
    except:
        title = 'MainPage'  # index.html only one

    with open(os.path.join(out_path, item), 'w') as f:
        f.write(template_soup.prettify(formatter='minimal').replace('$(TITLE)', title))


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

