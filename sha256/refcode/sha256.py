#!/usr/bin/env python3

import hashlib
data = "hello world!".encode('utf-8')
sha256_hash = hashlib.sha256(data).hexdigest()
print(sha256_hash)
