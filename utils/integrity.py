import os
import hashlib

def generate_file_hash(filepath):
    """Generate SHA-256 hash of a file."""
    if not os.path.exists(filepath):
        return None
    sha256 = hashlib.sha256()
    with open(filepath, 'rb') as f:
        while chunk := f.read(8192):
            sha256.update(chunk)
    return sha256.hexdigest()

def check_integrity(file_paths, expected_hashes):
    """
    Check if a list of files matches their expected hashes.
    Expected to be expanded to read a lockfile (e.g. integrity.lock)
    """
    for file, expected in zip(file_paths, expected_hashes):
        actual = generate_file_hash(file)
        if actual != expected:
            return False
    return True
