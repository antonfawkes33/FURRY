import os

import os

root_dir = r'C:\\Users\\Admin\\Desktop\\FURRY\\build'
# Exclude known binary extensions and directories
ignored_exts = {'.obj', '.tlog', '.pdb', '.lib', '.exe', '.bin', '.recipe', '.stamp', '.lastbuildstate'}

for root, dirs, files in os.walk(root_dir):
    for file in files:
        ext = os.path.splitext(file)[1].lower()
        if ext in ignored_exts:
            continue
            
        file_path = os.path.join(root, file)
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            if 'QtPie' in content:
                updated_content = content.replace('QtPie', 'FURRY')
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(updated_content)
                print(f"Updated: {file_path}")
        except Exception as e:
            print(f"Error processing {file_path}: {e}")
