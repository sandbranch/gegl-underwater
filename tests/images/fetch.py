#!/usr/bin/env python3
# Downloads the test photos listed in manifest.json from Wikimedia Commons
# into this folder, as 3000 pixel wide renditions. The photos are freely
# licensed (manifest.json names author, license and page of each) and stay
# out of git; this script makes the same set anywhere.
#   tests/images/fetch.py            all photos
#   tests/images/fetch.py --width 1500
import json
import os
import sys
import time
import urllib.parse
import urllib.request

here = os.path.dirname(os.path.abspath(__file__))
width = int(sys.argv[sys.argv.index('--width') + 1]) if '--width' in sys.argv else 3000
UA = {'User-Agent': 'gegl-underwater-test-fetch/0.1 (https://github.com/sandbranch/gegl-underwater)'}


def api(params):
    params.update(format='json', action='query')
    url = 'https://commons.wikimedia.org/w/api.php?' + urllib.parse.urlencode(params)
    return json.load(urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=60))


for photo in json.load(open(os.path.join(here, 'manifest.json'))):
    path = os.path.join(here, photo['file'])
    if os.path.exists(path):
        continue
    pages = api(dict(titles=photo['title'], prop='imageinfo', iiprop='url', iiurlwidth=width))['query']['pages']
    info = next(iter(pages.values()))['imageinfo'][0]
    data = urllib.request.urlopen(urllib.request.Request(info.get('thumburl', info['url']), headers=UA), timeout=120).read()
    with open(path + '.part', 'wb') as f:
        f.write(data)
    os.rename(path + '.part', path)
    print('%-26s %6d kB  %s (%s)' % (photo['file'], len(data) // 1024, photo['author'][:40], photo['license']))
    time.sleep(1)
