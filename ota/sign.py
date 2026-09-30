#!/usr/bin/env python3
"""Build + whole-file sign an OTA zip the way JB recovery (verifier.cpp) checks it.

Layout of the zip comment: [RSA-2048 PKCS#1 v1.5 SHA1 signature (256)] [footer (6)]
footer = sig_start(le16) ff ff comment_size(le16). Signed bytes = whole file up to,
but excluding, the EOCD comment-length field.

usage: sign.py <update-binary> <out.zip> [extra files...]
"""
import io, struct, sys, zipfile, pathlib, urllib.request, base64
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding

HERE = pathlib.Path(__file__).parent
KEY = HERE / "testkey.pk8"
AOSP = "https://android.googlesource.com/platform/build/+/refs/heads/main/target/product/security/"


def key():
    if not KEY.exists():  # public AOSP test key — the one otacerts.zip on the tablet trusts
        for f in ("testkey.pk8", "testkey.x509.pem"):
            data = urllib.request.urlopen(AOSP + f + "?format=TEXT").read()
            (HERE / f).write_bytes(base64.b64decode(data))
    return serialization.load_der_private_key(KEY.read_bytes(), None)


def sign(zbytes: bytes, k) -> bytes:
    assert zbytes[-22:-18] == b"PK\x05\x06" and zbytes[-2:] == b"\0\0", "zip must end in comment-less EOCD"
    body = zbytes[:-2]  # everything except the comment-length field
    sig = k.sign(body, padding.PKCS1v15(), hashes.SHA1())
    assert len(sig) == 256
    csize = 256 + 6
    comment = sig + struct.pack("<HBBH", csize, 0xFF, 0xFF, csize)
    assert b"PK\x05\x06" not in comment, "unlucky signature contains EOCD magic; change input"
    return body + struct.pack("<H", csize) + comment


def verify(data: bytes, k):
    """Mirror of verifier.cpp so a bad package never reaches the tablet."""
    f = data[-6:]
    assert f[2:4] == b"\xff\xff"
    sig_start, csize = f[0] | f[1] << 8, f[4] | f[5] << 8
    eocd = len(data) - (csize + 22)
    assert data[eocd:eocd + 4] == b"PK\x05\x06" and sig_start - 6 >= 256
    signed = data[:len(data) - csize - 2]
    sig = data[len(data) - 6 - 256:len(data) - 6]
    k.public_key().verify(sig, signed, padding.PKCS1v15(), hashes.SHA1())


if __name__ == "__main__":
    binary, out, *extra = sys.argv[1:]
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w", zipfile.ZIP_STORED) as z:
        z.write(binary, "META-INF/com/google/android/update-binary")
        z.writestr("META-INF/com/google/android/updater-script", "# unused, update-binary is custom\n")
        for e in extra:
            z.write(e, pathlib.Path(e).name)
    k = key()
    signed = sign(buf.getvalue(), k)
    verify(signed, k)
    pathlib.Path(out).write_bytes(signed)
    print(f"{out}: {len(signed)} bytes, signature verified locally")
