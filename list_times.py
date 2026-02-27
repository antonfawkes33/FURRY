import os
import datetime

root_dir = r'C:\Users\Admin\Desktop\FURRY\build'
results = []

for root, dirs, files in os.walk(root_dir):
    for file in files:
        if file.endswith('.vcxproj') or file.endswith('.slnx'):
            file_path = os.path.join(root, file)
            mtime = os.path.getmtime(file_path)
            dt = datetime.datetime.fromtimestamp(mtime)
            results.append((dt, file_path))

results.sort(reverse=True)
for dt, path in results:
    print(f"{dt} | {path}")
