#include "ble/pairing.h"
#if defined(TAMA_AGENT_MUSE)

#include <esp_random.h>
#include <esp_timer.h>
#include <mbedtls/ecp.h>
#include <mbedtls/gcm.h>
#include <mbedtls/md.h>
#include <mbedtls/platform_util.h>

#include <cstring>

#include "b64.h"

namespace tama {

namespace {

constexpr char kRecordLabel[] = "hatch-link ble setup v1";
constexpr char kSessionLabel[] = "hatch-link session id v1";
constexpr char kMobileToDevice[] = "mobile->device";
constexpr char kDeviceToMobile[] = "device->mobile";

constexpr size_t kPointBytes = 65;
constexpr size_t kNonceBytes = 16;
constexpr size_t kHashBytes = 32;
constexpr size_t kKeyBytes = 32;
constexpr size_t kSessionBytes = 16;
constexpr size_t kGcmNonceBytes = 12;
constexpr size_t kTagBytes = 16;
constexpr size_t kMaxPlain = 4096;

constexpr int64_t kFinishTimeoutUs = 60LL * 1000000;
constexpr int64_t kConfirmTimeoutUs = int64_t(muse::kConfirmTimeoutSeconds) * 1000000;
constexpr int64_t kReadyTimeoutUs = 120LL * 1000000;

constexpr uint8_t kInbound = 0;
constexpr uint8_t kOutbound = 1;

int random(void*, unsigned char* out, size_t len) {
  esp_fill_random(out, len);
  return 0;
}

const mbedtls_md_info_t* sha256() { return mbedtls_md_info_from_type(MBEDTLS_MD_SHA256); }

bool hash(const void* data, size_t len, uint8_t out[kHashBytes]) {
  return mbedtls_md(sha256(), static_cast<const uint8_t*>(data), len, out) == 0;
}

bool hmac(const uint8_t* key, size_t keyLen, const uint8_t* data, size_t len,
          uint8_t out[kHashBytes]) {
  return mbedtls_md_hmac(sha256(), key, keyLen, data, len, out) == 0;
}

bool expand(const uint8_t prk[kKeyBytes], const char* info, uint8_t out[kKeyBytes]) {
  std::string block(info);
  block.push_back('\x01');
  return hmac(prk, kKeyBytes, reinterpret_cast<const uint8_t*>(block.data()), block.size(), out);
}

std::string text(const uint8_t* data, size_t len) { return b64::encodeUrl(data, len); }

bool bytes(const std::string& encoded, uint8_t* out, size_t len) {
  std::string raw;
  if (!b64::decodeUrl(encoded, raw) || raw.size() != len) return false;
  std::memcpy(out, raw.data(), len);
  return true;
}

std::string transcript(const muse::Device& device, const std::string& mobilePub,
                       const std::string& devicePub, const std::string& mobileNonce,
                       const std::string& deviceNonce) {
  return std::string("hatch-link-pairing-v5") + "\nversion=" + std::to_string(muse::kPairingVersion) +
         "\ninitiator_role=mobile\nresponder_role=link" + "\ndevice_id=" + device.id +
         "\nnode_id=" + device.node + "\nmac=" + device.mac + "\nmodel=" + muse::kPairingModel +
         "\nfirmware_version=" + device.firmware +
         "\nselected_cipher_suite=" + muse::kPairingSuite + "\npairing_auth=" + muse::kPairingAuth +
         "\npairing_auth_epoch=0" + "\npairing_policy=" + muse::kPairingPolicy +
         "\nconfirm_timeout_seconds=" + std::to_string(muse::kConfirmTimeoutSeconds) +
         "\nmobile_pub=" + mobilePub + "\ndevice_pub=" + devicePub +
         "\nmobile_nonce=" + mobileNonce + "\ndevice_nonce=" + deviceNonce;
}

std::string aad(const std::string& session, uint8_t direction, uint64_t counter) {
  return std::string(kRecordLabel) + "|" + session + "|" + (direction == kInbound ? "m2d" : "d2m") +
         "|" + std::to_string(counter);
}

void nonce(uint8_t direction, uint64_t counter, uint8_t out[kGcmNonceBytes]) {
  std::memset(out, 0, kGcmNonceBytes);
  out[0] = direction;
  for (int i = 0; i < 8; ++i) out[11 - i] = static_cast<uint8_t>(counter >> (8 * i));
}

bool ecdh(const uint8_t mobile[kPointBytes], uint8_t devicePub[kPointBytes],
          uint8_t secret[kKeyBytes]) {
  mbedtls_ecp_group group;
  mbedtls_ecp_point peer, pub, shared;
  mbedtls_mpi priv;
  mbedtls_ecp_group_init(&group);
  mbedtls_ecp_point_init(&peer);
  mbedtls_ecp_point_init(&pub);
  mbedtls_ecp_point_init(&shared);
  mbedtls_mpi_init(&priv);

  size_t written = 0;
  int rc = mbedtls_ecp_group_load(&group, MBEDTLS_ECP_DP_SECP256R1);
  if (rc == 0) rc = mbedtls_ecp_point_read_binary(&group, &peer, mobile, kPointBytes);
  if (rc == 0) rc = mbedtls_ecp_check_pubkey(&group, &peer);
  if (rc == 0) rc = mbedtls_ecp_gen_keypair(&group, &priv, &pub, random, nullptr);
  if (rc == 0)
    rc = mbedtls_ecp_point_write_binary(&group, &pub, MBEDTLS_ECP_PF_UNCOMPRESSED, &written,
                                        devicePub, kPointBytes);
  if (rc == 0 && written != kPointBytes) rc = -1;
  if (rc == 0) rc = mbedtls_ecp_mul(&group, &shared, &priv, &peer, random, nullptr);
  if (rc == 0) rc = mbedtls_mpi_write_binary(&shared.MBEDTLS_PRIVATE(X), secret, kKeyBytes);

  mbedtls_mpi_free(&priv);
  mbedtls_ecp_point_free(&shared);
  mbedtls_ecp_point_free(&pub);
  mbedtls_ecp_point_free(&peer);
  mbedtls_ecp_group_free(&group);
  return rc == 0;
}

}  // namespace

bool Pairing::hello(const muse::Hello& hello, muse::Ready& out) {
  reset();
  uint8_t mobilePub[kPointBytes];
  uint8_t mobileNonce[kNonceBytes];
  if (!bytes(hello.pub, mobilePub, kPointBytes) || mobilePub[0] != 0x04 ||
      !bytes(hello.nonce, mobileNonce, kNonceBytes))
    return false;

  uint8_t devicePub[kPointBytes];
  uint8_t deviceNonce[kNonceBytes];
  uint8_t secret[kKeyBytes];
  if (!ecdh(mobilePub, devicePub, secret)) return false;
  esp_fill_random(deviceNonce, sizeof(deviceNonce));

  out.pub = text(devicePub, kPointBytes);
  out.nonce = text(deviceNonce, kNonceBytes);
  const std::string record = transcript(device_, text(mobilePub, kPointBytes), out.pub,
                                        text(mobileNonce, kNonceBytes), out.nonce);

  uint8_t transcriptHash[kHashBytes];
  uint8_t salt[kHashBytes];
  uint8_t prk[kHashBytes];
  uint8_t sessionSecret[kKeyBytes];
  uint8_t sessionHash[kHashBytes];
  uint8_t saltInput[kNonceBytes * 2 + kHashBytes];
  std::string sessionInput(kSessionLabel);

  bool ok = hash(record.data(), record.size(), transcriptHash);
  if (ok) {
    std::memcpy(saltInput, mobileNonce, kNonceBytes);
    std::memcpy(saltInput + kNonceBytes, deviceNonce, kNonceBytes);
    std::memcpy(saltInput + 2 * kNonceBytes, transcriptHash, kHashBytes);
    std::string label(kRecordLabel);
    label.push_back('\x01');
    ok = hash(saltInput, sizeof(saltInput), salt) && hmac(salt, kHashBytes, secret, kKeyBytes, prk) &&
         hmac(prk, kHashBytes, reinterpret_cast<const uint8_t*>(label.data()), label.size(),
              sessionSecret) &&
         expand(sessionSecret, kMobileToDevice, rx_) && expand(sessionSecret, kDeviceToMobile, tx_);
  }
  if (ok) {
    sessionInput.append(reinterpret_cast<const char*>(transcriptHash), kHashBytes);
    sessionInput.append(reinterpret_cast<const char*>(secret), kKeyBytes);
    ok = hash(sessionInput.data(), sessionInput.size(), sessionHash);
  }

  mbedtls_platform_zeroize(secret, sizeof(secret));
  mbedtls_platform_zeroize(prk, sizeof(prk));
  mbedtls_platform_zeroize(sessionSecret, sizeof(sessionSecret));
  mbedtls_platform_zeroize(&sessionInput[0], sessionInput.size());
  if (!ok) {
    reset();
    return false;
  }

  session_ = text(sessionHash, kSessionBytes);
  out.transcript = text(transcriptHash, kHashBytes);
  out.session = session_;
  advance(Phase::Finishing, kFinishTimeoutUs);
  return true;
}

bool Pairing::open(const muse::Sealed& sealed, std::string& plain) {
  if (!keyed() || expired() || sealed.session != session_ || sealed.counter != rxCounter_)
    return false;
  std::string cipher;
  std::string tag;
  if (!b64::decodeUrl(sealed.ciphertext, cipher) || cipher.size() > kMaxPlain ||
      !b64::decodeUrl(sealed.tag, tag) || tag.size() != kTagBytes)
    return false;

  uint8_t iv[kGcmNonceBytes];
  nonce(kInbound, rxCounter_, iv);
  const std::string extra = aad(session_, kInbound, rxCounter_);
  plain.assign(cipher.size(), '\0');

  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);
  int rc = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, rx_, kKeyBytes * 8);
  if (rc == 0)
    rc = mbedtls_gcm_auth_decrypt(&gcm, cipher.size(), iv, sizeof(iv),
                                  reinterpret_cast<const uint8_t*>(extra.data()), extra.size(),
                                  reinterpret_cast<const uint8_t*>(tag.data()), kTagBytes,
                                  reinterpret_cast<const uint8_t*>(cipher.data()),
                                  reinterpret_cast<uint8_t*>(&plain[0]));
  mbedtls_gcm_free(&gcm);
  if (rc != 0) {
    plain.clear();
    return false;
  }
  ++rxCounter_;
  return true;
}

bool Pairing::seal(const std::string& plain, muse::Sealed& out) {
  if (!keyed()) return false;
  uint8_t iv[kGcmNonceBytes];
  nonce(kOutbound, txCounter_, iv);
  const std::string extra = aad(session_, kOutbound, txCounter_);
  std::string cipher(plain.size(), '\0');
  uint8_t tag[kTagBytes];

  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);
  int rc = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, tx_, kKeyBytes * 8);
  if (rc == 0)
    rc = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, plain.size(), iv, sizeof(iv),
                                   reinterpret_cast<const uint8_t*>(extra.data()), extra.size(),
                                   reinterpret_cast<const uint8_t*>(plain.data()),
                                   reinterpret_cast<uint8_t*>(&cipher[0]), kTagBytes, tag);
  mbedtls_gcm_free(&gcm);
  if (rc != 0) return false;

  out.session = session_;
  out.counter = txCounter_++;
  out.ciphertext = text(reinterpret_cast<const uint8_t*>(cipher.data()), cipher.size());
  out.tag = text(tag, kTagBytes);
  return true;
}

bool Pairing::finish() {
  if (phase_ != Phase::Finishing || expired() || rxCounter_ != 1) return false;
  advance(Phase::Confirming, kConfirmTimeoutUs);
  return true;
}

bool Pairing::confirm() {
  if (phase_ != Phase::Confirming || expired()) return false;
  advance(Phase::Ready, kReadyTimeoutUs);
  return true;
}

bool Pairing::expired() const {
  return phase_ != Phase::Idle && esp_timer_get_time() > deadline_;
}

void Pairing::reset() {
  mbedtls_platform_zeroize(rx_, sizeof(rx_));
  mbedtls_platform_zeroize(tx_, sizeof(tx_));
  session_.clear();
  rxCounter_ = 0;
  txCounter_ = 0;
  deadline_ = 0;
  phase_ = Phase::Idle;
}

void Pairing::advance(Phase phase, int64_t timeoutUs) {
  phase_ = phase;
  deadline_ = esp_timer_get_time() + timeoutUs;
}

}  // namespace tama

#endif
