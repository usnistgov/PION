# ndnob-authenticator

## Obtain Authenticator Certificate

```bash
export NDNPH_KEYCHAIN=/tmp/ndnob-keychain
export NDNPH_UPLINK_UDP=192.168.5.36
export NDNPH_UPLINK_UDP_PORT=6363

NETWORK_PREFIX=/my-network
CA_PROFILE_FILE=/tmp/ndnob-ca-profile.data

AUTHENTICATOR_KEYSLOT=a
AUTHENTICATOR_NAME=${NETWORK_PREFIX}/32=onboarding-authenticator/${AUTHENTICATOR_KEYSLOT}${RANDOM}

ndnph-keychain keygen ${AUTHENTICATOR_KEYSLOT} ${AUTHENTICATOR_NAME} >/dev/null
ndnph-ndncertclient -P ${CA_PROFILE_FILE} -i ${AUTHENTICATOR_KEYSLOT} | ndnph-keychain certimport ${AUTHENTICATOR_KEYSLOT}
ndnph-keychain certinfo ${AUTHENTICATOR_KEYSLOT}
```

## Execute Authenticator

```bash
export NDNPH_KEYCHAIN=/tmp/ndnob-keychain
export NDNPH_UPLINK_UDP=192.168.5.36
export NDNPH_UPLINK_UDP_PORT=46363

NETWORK_PREFIX=/my-network
DEVICE_NAME=${NETWORK_PREFIX}/device${RANDOM}

ndnob-authenticator ${AUTHENTICATOR_KEYSLOT} ${CA_PROFILE_FILE} ${DEVICE_NAME}
```
