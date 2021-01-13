# ndnob-authenticator

## Establish CA and Authenticator Certificate

```bash
nfd-start

export NDNTS_KEYCHAIN=/tmp/ca-keychain
export NDNTS_NFDREG=1
export NDNPH_KEYCHAIN=/tmp/ndnob-keychain

NETWORK_PREFIX=/my-network
CA_PROFILE_FILE=/tmp/profile.data
CA_REPO_PATH=/tmp/ca-repo

AUTHENTICATOR_KEYSLOT=a
AUTHENTICATOR_NAME=${NETWORK_PREFIX}/32=onboarding-authenticator/${AUTHENTICATOR_KEYSLOT}${RANDOM}

CACERT=$(ndntssec gen-key ${NETWORK_PREFIX})
ndntssec ndncert03-make-profile --out ${CA_PROFILE_FILE} --prefix ${NETWORK_PREFIX} --cert ${CACERT} --valid-days 60
ndntssec ndncert03-show-profile --profile ${CA_PROFILE_FILE}
ndntssec ndncert03-ca --profile ${CA_PROFILE_FILE} --store ${CA_REPO_PATH} --challenge nop

ndnph-keychain keygen ${AUTHENTICATOR_KEYSLOT} ${AUTHENTICATOR_NAME} >/dev/null
ndnph-ndncertclient -P ${CA_PROFILE_FILE} -i ${AUTHENTICATOR_KEYSLOT} | ndnph-keychain certimport ${AUTHENTICATOR_KEYSLOT}
ndnph-keychain certinfo ${AUTHENTICATOR_KEYSLOT}

pkill -f ndncert03-ca
nfd-stop
```

## Execute Authenticator

```bash
export NDNPH_UPLINK_UDP=192.0.2.1 # device IPv4 address
DEVICE_NAME=${NETWORK_PREFIX}/device-${RANDOM}

ndnob-authenticator ${AUTHENTICATOR_KEYSLOT} ${CA_PROFILE_FILE} ${DEVICE_NAME}
```
