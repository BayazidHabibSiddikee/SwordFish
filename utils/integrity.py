import hashlib
import os
import sys

# Secret cipher to be appended to the QR code and used for verification
CIPHER_TAG = b"SWORDFISH-SECURE-ID-2026"

def get_file_hash(path):
    """Calculate SHA-256 hash of a file."""
    hasher = hashlib.sha256()
    with open(path, 'rb') as f:
        while chunk := f.read(8192):
            hasher.update(chunk)
    return hasher.hexdigest()

def get_dir_manifest(dir_path):
    """Get a sorted list of files and their hashes in a directory."""
    manifest = {}
    for root, _, files in os.walk(dir_path):
        if "__pycache__" in root:
            continue
        for file in sorted(files):
            path = os.path.join(root, file)
            # Relative path for consistency
            rel_path = os.path.relpath(path, dir_path)
            manifest[rel_path] = get_file_hash(path)
    return manifest

def tag_file(file_path):
    """Append the cipher tag to a file if not already present."""
    if not os.path.exists(file_path):
        return
    with open(file_path, 'rb') as f:
        data = f.read()
    if not data.endswith(CIPHER_TAG):
        with open(file_path, 'ab') as f:
            f.write(CIPHER_TAG)

def verify_integrity(src_dir, qr_path, icon_path, expected_manifest_file):
    """Verify that the src directory, QR code, and Icon haven't changed."""
    import json
    
    if not os.path.exists(expected_manifest_file):
        return False, "Manifest missing"

    with open(expected_manifest_file, 'r') as f:
        expected = json.load(f)

    # 1. Check QR Code tag
    if os.path.exists(qr_path):
        with open(qr_path, 'rb') as f:
            data = f.read()
        if not data.endswith(CIPHER_TAG):
            return False, "QR Code integrity compromised (Cipher tag missing)"
    else:
        return False, "QR Code missing"

    # 2. Check Icon tag
    if os.path.exists(icon_path):
        with open(icon_path, 'rb') as f:
            data = f.read()
        if not data.endswith(CIPHER_TAG):
            return False, "Icon integrity compromised (Cipher tag missing)"
    else:
        return False, "Icon missing"

    # 3. Check src directory manifest
    current_manifest = get_dir_manifest(src_dir)
    
    if current_manifest != expected:
        # Check for new files or modified files
        new_files = set(current_manifest.keys()) - set(expected.keys())
        missing_files = set(expected.keys()) - set(current_manifest.keys())
        modified_files = [k for k in current_manifest if k in expected and current_manifest[k] != expected[k]]
        
        err = "Integrity violation:"
        if new_files: err += f"\n- Unauthorized files found: {', '.join(new_files)}"
        if missing_files: err += f"\n- Missing files: {', '.join(missing_files)}"
        if modified_files: err += f"\n- Modified files: {', '.join(modified_files)}"
        return False, err

    return True, "Integrity OK"

if __name__ == "__main__":
    # Generator mode
    import json
    ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    SRC = os.path.join(ROOT, "src")
    QR = os.path.join(SRC, "qrcode.png")
    ICON = os.path.join(ROOT, "icon.png")
    MANIFEST = os.path.join(ROOT, ".manifest.json")
    
    print("Tagging security files...")
    tag_file(QR)
    tag_file(ICON)
    
    print("Generating manifest...")
    manifest = get_dir_manifest(SRC)
    with open(MANIFEST, 'w') as f:
        json.dump(manifest, f, indent=2)
    print(f"Manifest saved to {MANIFEST}")
