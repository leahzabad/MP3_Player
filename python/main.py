import requests
from PIL import Image

#spotufy api handling
token_url = "https://accounts.spotify.com/api/token"
token_data = {"grant_type": "client_credentials",
        "client_id": client_id,
        "client_secret": client_secret}
token_headers = {"Content-Type": "application/x-www-form-urlencoded"}

key = requests.post(token_url, data=token_data, headers=token_headers)
access_dict = key.json()
#print(access_dict)

def get_album_id(name, access_token, artist=None):
    query = f"album:{name}"
    if artist:
        query += f" artist:{artist}"
    resp = requests.get(
        "https://api.spotify.com/v1/search",
        headers={"Authorization": access_token},
        params={"q": query, "type": "album", "limit": 1}
    )
    results = resp.json()
    items = results["albums"]["items"]
    if not items:
        return None
    return items[0]["id"]

tempstr = access_dict['token_type'] + " " + access_dict['access_token']
album_header = {"Authorization": tempstr}
album_url_b = "https://api.spotify.com/v1/albums/"
album_name = input("Album name: ")
artist_name = input("Artist (optional, press enter to skip): ") or None
album_ID = get_album_id(album_name, tempstr, artist_name)
album_url = album_url_b + album_ID

#getting the info
album_data = requests.get(album_url, headers=album_header)
album_dict = album_data.json()
print(album_dict)
print(album_dict['name'])
print(album_dict['images'][0]['url'])
# download album art
albumname = album_dict['name'];
img_url = album_dict['images'][-1]['url']
img_data = requests.get(img_url).content
filename = f"{album_ID}.jpg"
with open(filename, "wb") as f:
    f.write(img_data)
print((album_dict['artists'][0])['name'])
tracks_nb = album_dict['tracks']['total']
for i in range(0, tracks_nb):
    print((album_dict['tracks']['items'][i])['name'])


#formatting album art to bitmap
img = Image.open(filename).convert("RGB")
newsize = (108, 108)
fimg = img.resize(newsize)
width, height = fimg.size
pixels = fimg.load()

rgb565 = []
for y in range(height):
    for x in range(width):
        r, g, b = pixels[x, y]
        val = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        rgb565.append(val)

with open("album_art.h", "w") as f:
    f.write(f"#define ALBUM_ART_WIDTH {width}\n")
    f.write(f"#define ALBUM_ART_HEIGHT {height}\n")
    f.write("const uint16_t albumArt[] PROGMEM = {\n")
    for i in range(0, len(rgb565), 16):
        f.write("  " + ", ".join(str(v) for v in rgb565[i:i+16]) + ",\n")
    f.write("};\n")



#https://open.spotify.com/album/4lkJ6i3LDK8HvcU2tPWX9k?si=Ac3H9qA_SZyK2SlBEliDCw
#https://open.spotify.com/album/0A1c1SWNQUub5c1BkVzam7?si=XE0FRVDXT-2OFJ0V_YQrPQ
#https://open.spotify.com/album/71EVZx82GopCyTNRZVkZSk?si=337MJyLaR7ySGU333y7aHQ