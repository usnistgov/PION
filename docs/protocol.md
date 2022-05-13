# PION Onboarding Protocol Specification, version 1

This document defines an onboarding procedure for IoT devices over the Named Data Networking (NDN) protocol.
This procedure allows a homeowner or building manager to securely add a newly acquired IoT device to an existing NDN network.

## Participants, Assumptions, and Goals

There are four participants in this procedure:

1. The infrastructure network, such as a Wi-Fi (IEEE 802.11) access point.
   *NC* denotes the credentials to access this network, such as a Wi-Fi password.
2. **A** is a certificate authority running the [NDNCERT v0.3 protocol](https://github.com/named-data/ndncert/wiki/NDNCERT-Protocol-0.3).
3. **H** is an authenticator.
   It is a handheld terminal (e.g., a smartphone) with rich user interaction capabilities.
4. **D** is a newly acquired device (e.g., a temperature sensor, a connected light bulb).
   It has limited user interaction capabilities.
   At one point during the onboarding procedure, and for a limited period of time, **D** will acquire a temporary identity **T**.

Before the onboarding procedure begins, it is assumed that:

* **A** owns the key pair *AK* and publishes its CA profile *Aprofile*.
  * The CA profile is a (signed) Data packet that contains the CA prefix and certificate (*Acert*).
* **H** owns the key pair *HK*, and has a certificate *Hcert* issued by **A**.
  * The subject name of *Hcert* contains a `32=pion-authenticator` component to authorize **H** as an authenticator.
* **H** knows *NC* and *Aprofile*.
* **A** and **H** have (loosely) synchronized clocks; **D** does not have to be synchronized or even have a stable clock source at all.
* **D** has been restored to factory settings.
* **H** can establish a direct connection to **D**.
  Necessary credentials, if any, are part of **D**'s factory settings.
  * No assumptions are made on the security of this direct connection.
  * It is possible that previous owners of **D** know the factory settings, such as the direct connection credentials.

At the successful completion of the procedure, it is expected that:

* **D**, and only **D**, learns *NC*.
* **D** obtains *Acert*.
* **A** issues a certificate to **D**.
* **A** does not issue a certificate to anyone other than **D**.
* **D** receives its own certificate (*Dcert*) signed by **A**.

### Example Names

The following are examples of names and name prefixes used in the PION protocol.

* Network prefix: `/example/my-network`
* CA prefix in *Aprofile*: `/example/my-network/CA`
* Subject name of *Acert*: `/example/my-network`
* Subject name of *Hcert*: `/example/my-network/32=pion-authenticator/handheld`
* **D**'s identity name: `/example/my-network/new-device`
* Subject name of *Tcert*: `/example/my-network/32=pion-authenticated/new-device`

### Limitations

* **A** is assumed to be the root trust anchor.
  If *Aprofile* is signed by a higher authority, **H** needs to provide the root trust anchor and trust schema to **D**, so that **D** can validate *Aprofile* and *Dcert*.

## Protocol Specification

### Part 1: Obtain D's Factory Settings

**H** learns **D**'s factory settings through offline means.
For example, **H** may scan a QR code printed on a sticker placed on **D** or its packaging.

### Part 2: Authenticate D and Provide Temporary Credential

**H** establishes a direct connection to **D**.
This can be achieved in various ways depending on the communication technologies supported by **H** and **D**.
A few possibilities are outlined in a later section of this document.

Once a direct connection is established, the user must perform an out-of-band (OOB) password entry procedure, so that both **H** and **D** possess a weak shared secret *PW*.
The protocol does not mandate how *PW* should be shared between **H** and **D**.
Typically, either **D** or **H** generates and displays a password, and the user manually enters that password into the other entity.
A few alternative methods for the OOB password entry are outlined in a later section of this document.
The password can be short and have low complexity.
20-30 bits of entropy are usually considered sufficient, equivalent to a 7-digit PIN or a 5-character alphanumeric string randomly sampled from a uniform distribution.
It is recommended to regenerate the displayed password periodically (e.g., every 60 seconds).

The user may also specify an identity name for **D**.
If an identity name is not specified, **H** can automatically generate one for **D**.

**H** generates a 64-bit random session identifier *SID*.
When *SID* appears in a cryptographic context, it is encoded as an 8-octet binary string.
When *SID* appears as a name component, it is encoded as a GenericNameComponent containing this binary string as value.

**H** and **D** initiate an instance of the [SPAKE2 password-based key exchange protocol](https://www.ietf.org/archive/id/draft-irtf-cfrg-spake2-26.html), with the following settings:

* The ciphersuite is: SPAKE2-P256-SHA256-HKDF-HMAC.
* **H** takes the role of A. Its identity is the SHA-256 digest of *Hcert*.
* **D** takes the role of B. Its identity is absent.
* *SPAKE2-w* = *SHA-256(PW) mod SPAKE2-p*.
  * The SPAKE2 specification recommends using a memory-hard hash function (MHF), such as scrypt, in this calculation.
    However, low-end IoT devices may be incapable of running a MHF, so SHA-256 is used instead.
* *SPAKE2-AAD* is the session identifier *SID*.

Using this SPAKE2 instance, **H** and **D** exchange messages to establish a shared secret *SPAKE2-Ke*.
Then, **H** provides a temporary credential to **D**, which is used in part 3 of the PION protocol.

> Note: the "*SPAKE2-*" prefix denotes a variable defined in the SPAKE2 protocol, to distinguish them from other variables used in PION. For example, "*SPAKE2-AAD*" denotes the additional authenticated data used in SPAKE2.

#### Additional Requirements

Symmetric encryption uses AES-GCM with a 128-bit key, a fresh 12-byte IV, and *SID* as additional data.
The IV construction follows the recommendation in the NDNCERT protocol.
Ciphertext, IV, and authentication tag are transmitted together.

Most messages do not have NDN signatures, but they are associated to the session via the AEAD feature of AES-GCM.
Data packets may use [NullSignature](https://redmine.named-data.net/projects/ndn-tlv/wiki/NullSignature).

Any SPAKE2 or AES-GCM error causes the receiving entity to abort the protocol.
If the receiving entity is **D**, it should respond with an error message: a Data packet whose [ContentType](https://redmine.named-data.net/projects/ndn-tlv/wiki/ContentType) is `Nack`.
This message should not reveal any specific information about the cryptographic error that may be leveraged by an attacker.

This part of the procedure must be completed within a short preconfigured time limit (e.g., 30 seconds), since sending/receiving the first message.
Both **H** and **D** should enforce this time limit.

While these messages are exchanged, password regeneration and password entry should be suspended.
In case the procedure fails, the OOB password must not be reused, a fresh password must be generated and entered.

#### Message 1

**H** starts an instance of the SPAKE2 protocol with the role of A, and computes its public share *SPAKE2-pA*.

**H** transmits an Interest with:

* Name: `/localhop/32=pion/SID/pake`
* Parameters, not encrypted:
  * *SPAKE2-pA*
  * Name and SHA-256 digest of *Hcert*
  * Optionally, a forwarding hint to reach **H** for Data retrievals

The parameters are encoded as follows:

```abnf
message1-parameters = spake2-pa
                      authenticator-cert-name
                      [ForwardingHint]

spake2-pa = spake2-pa-type TLV-LENGTH *OCTET
spake2-pa-type = %xfd.8f.01

authenticator-cert-name = authenticator-cert-name-type TLV-LENGTH
                          Name ; must include implicit digest component
authenticator-cert-name-type = %xfd.8f.0d

; Name and ForwardingHint are defined in the NDN Packet Format specification.
```

Upon receiving the Interest, **D** performs the following steps and immediately aborts the procedure if any step fails:

1. Start an instance of SPAKE2 taking the role of B.
2. Compute its public share *SPAKE2-pB*.
3. Process **H**'s public share *SPAKE2-pA*.
4. Generate its key confirmation message *SPAKE2-cB*.

#### Message 2

**D** replies to message 1 with a Data packet containing:

* Payload, not encrypted:
  * *SPAKE2-pB*
  * *SPAKE2-cB*

The parameters are encoded as follows:

```abnf
message2-parameters = spake2-pb
                      spake2-cb

spake2-pb = spake2-pb-type TLV-LENGTH *OCTET
spake2-pb-type = %xfd.8f.03

spake2-cb = spake2-cb-type TLV-LENGTH *OCTET
spake2-cb-type = %xfd.8f.05
```

Upon receiving this Data packet, **H** performs the following steps and immediately aborts the procedure if any step fails:

1. Process **D**'s public share *SPAKE2-pB* in the existing SPAKE2 instance.
2. Generate its key confirmation message *SPAKE2-cA*.
3. Verify **D**'s key confirmation message *SPAKE2-cB*.

#### Message 3

**H** transmits an Interest with:

* Name: `/localhop/32=pion/SID/confirm`
* Parameters, not encrypted:
  * *SPAKE2-cA*
* Parameters, encrypted by *SPAKE2-Ke*:
  * *NC*
  * Name and SHA-256 digest of *Aprofile*
  * Identity name of **D**
  * Current timestamp

The parameters are encoded as follows:

```abnf
message3-parameters = spake2-ca
                      encrypted-message

spake2-ca = spake2-ca-type TLV-LENGTH *OCTET
spake2-ca-type = %xfd.8f.07

message3-plaintext = nc
                     ca-profile-name
                     device-name
                     timestamp

nc = nc-type TLV-LENGTH *OCTET
nc-type = %xfd.8f.09

ca-profile-name = ca-profile-name-type TLV-LENGTH
                  Name ; must include implicit digest component
ca-profile-name-type = %xfd.8f.0b

device-name = device-name-type TLV-LENGTH Name
device-name-type = %xfd.8f.0f

timestamp = TimestampNameComponent

; encrypted-message is defined by the NDNCERT protocol.
; TimestampNameComponent is defined by the NDN naming conventions.
```

Upon receiving the Interest, **D** performs the following steps and immediately aborts the procedure if any step fails:

1. Verify **H**'s key confirmation message *SPAKE2-cA* in the existing SPAKE2 instance.
2. If **D** does not have its own clock source, it initializes its clock to the provided timestamp.
   Otherwise, it checks that the received timestamp is within 120 seconds of its own clock.
3. Retrieve *Aprofile* and *Hcert*, and verify them against the provided digests.
4. Verify that *Hcert* and *Acert* are unexpired.
5. Verify that *Hcert* is a certificate issued by **A**, using the *Acert* enclosed in *Aprofile*.

#### Message 4

**D** generates a temporary key pair *TK*.
It then uses this key pair to create a self-signed certificate request *Treq*.
The subject name of *Treq* is the concatenation of:

1. The leading portion of the subject name of *Hcert* up to and excluding the `32=pion-authenticator` component.
2. A fixed component `32=pion-authenticated`.
3. The suffix of **D**'s identity name obtained by stripping the leading components already appearing in the first part.

**D** replies to message 3 with a Data packet containing:

* Payload, encrypted by *SPAKE2-Ke*:
  * *Treq*

The parameters are encoded as follows:

```abnf
message4-parameters = encrypted-message

message4-plaintext = treq

treq = treq-type TLV-LENGTH Certificate
treq-type = %xfd.8f.11

; Certificate is defined by the NDN Certificate Format specification in ndn-cxx.
```

Upon receiving the Data, **H** checks that the subject name of *Treq* has the correct structure that matches the identity name provided to **D** in message 3.
The protocol is aborted if this is not the case.

#### Message 5

**H** signs a certificate *Tcert* for the public key enclosed in *Treq*.
The subject name of *Tcert* is the same as *Treq*.
The validity period of *Tcert* should be on the order of 10-20 minutes, long enough for **D** to complete the NDNCERT part of the protocol and take into account clock inaccuracy.
The certificate is signed by *HKpri* and has a KeyLocator that identifies *Hcert*.

**H** transmits an Interest with:

* Name: `/localhop/32=pion/SID/credential`
* Parameters, encrypted by *SPAKE2-Ke*:
  * Name and SHA-256 digest of *Tcert*

The parameters are encoded as follows:

```abnf
message5-parameters = encrypted-message

message5-plaintext = issued-cert-name

; issued-cert-name is defined by the NDNCERT protocol.
```

Upon receiving the Interest, **D** performs the following steps and immediately aborts the procedure if any step fails:

1. Retrieve *Tcert* and verify it against the provided digest.
2. Verify that *Tcert* and *Hcert* are unexpired.
3. Verify that *Tcert* is a certificate issued by **H**, according to *Hcert*.

#### Message 6

**D** replies to message 5 with an empty Data packet as acknowledgement.
The packet is signed by *TKpri* and has a KeyLocator that identifies *Tcert*.

When **H** receives the acknowledgement Data, it closes the direct connection to **D**.

**D** should maintain the direct connection for a short period of time (e.g., 5 seconds) to accommodate any potential retransmissions of the acknowledgement Data.
This waiting period may end early if **D** detects that **H** has disconnected.
Eventually, **D** closes the direct connection and connects to the infrastructure network.

### Part 3: Issue Certificate to D

**D** initiates an NDNCERT certificate request with **A**, as identified in *Aprofile*.
During the NDNCERT procedure:

1. The PROBE step should be skipped.
2. In the NEW step, **A** must offer the [proof-of-possession challenge](https://github.com/named-data/ndncert/wiki/NDNCERT-Protocol-0.3-Challenges#3-proof-of-possession-challenge).
3. In the CHALLENGE step, **D** should complete the proof-of-possession challenge using *Tcert* as its existing certificate.

**A** approves the challenge and issues a certificate to **D** if and only if all of these conditions are met:

* The subject name of *Tcert* contains the `32=pion-authenticated` component.
* The KeyLocator of *Tcert* identifies a certificate issued by **A** and whose subject name contains the `32=pion-authenticator` component.
* *Tcert* is signed by its issuer (i.e., **H**).
* The prefix of *Tcert*'s subject name before `32=pion-authenticated` equals the prefix of the issuer's subject name before `32=pion-authenticator`.
* The identity name requested by **D** in the NEW step matches the subject name of *Tcert* with the component `32=pion-authenticated` removed.

Protocol timeouts and challenge retry limits during this phase are enforced according to the NDNCERT specification.

## Direct Connection

PION assumes that **D** initially does not know *NC*, the credentials to access the infrastructure network.
It is necessary for **D** to learn *NC*, so that it can talk to **A** and other nodes in the network.
In today's marketplace, many smart home devices solve this problem by letting their mobile app broadcast *NC* in plaintext, which weakens the security of the infrastructure network.
PION chooses a different approach to protect *NC*: perform the initial authentication over a direct connection between **H** and **D**, and provide *NC* in an encrypted form over this direct connection.

This section discusses a few methods of establishing this direct connection.

### Wi-Fi and Bluetooth

Generally, **H** is a smartphone that supports Wi-Fi and Bluetooth.
When **H** and **D** support the same communication protocol, they can connect directly.

If **D** supports Wi-Fi, it can establish a Wi-Fi access point.
The SSID and passphrase for this access point are part of **D**'s factory settings.
**H** can then connect to the access point.

If **D** supports Bluetooth Low Energy (BLE), it can act as a BLE peripheral.
The BLE address is part of **D**'s factory settings or can be discovered via scanning.
**H** can then exchange messages with the BLE peripheral without association or encryption.

### 802.15.4, LoRa, and others

If **H** is a smartphone, typically it will not support protocols like 802.15.4 or LoRa.
When **H** and **D** do not support the same communication protocol, they must communicate via a gateway or proxy.

PION makes no assumptions on the security of the direct connection.
Therefore, the gateway and/or any other intermediate nodes do not need to be trusted.

To facilitate forwarding via a gateway, **D** may optionally define, as part of its factory settings, a factory-generated name prefix *DF* unique to each device.
Then, **H** can use `/DF/32=pion` in place of `/localhop/32=pion` in Interest names, and set a forwarding hint so that the Interests can reach the gateway.

## OOB Password Entry

An out-of-band password, along with the SPAKE2 protocol, guarantees that **D** and **H** are talking to each other without a man-in-the-middle.
The password entry method is highly dependent on **D**'s user interaction capabilities.
The minimum requirement is either a single button or a single light source.

Generally, **D** should have some hardware-based "reset" mechanism to initiate an onboarding procedure.
After it has been reset, **D** can either start generating and displaying passwords, or start accepting password entry.

### Single Button Interaction

**H** generates an 8-bit random number *PW*.
**H** converts *PW* to a base-4 number and adds 1 to each digit, to obtain a 4-digit code where each digit is either 1, 2, 3, or 4.
**H** displays this 4-digit code.

The user must enter this code into **D** using the button: for each digit, press the button 1, 2, 3, or 4 times within 1 second, and wait 2-3 seconds between digits.
**D** decodes the button presses into the 4-digit code and reconstructs *PW*.

Due to usability limitations, this method is able to provide only 8 bits of entropy.

### Single Light Interaction

**D** generates a 32-bit random number *PW*.
**D** repeatedly blinks out *PW*:

1. Turn on the light for 50ms and turn off the light for 50ms, repeat 6 times.
2. For each bit 0, turn on the light for 20ms and turn off the light for 80ms.
   For each bit 1, turn on the light for 80ms and turn off the light for 20ms.

The user uses **H**'s video camera to record the blink pattern and recognize *PW*.
Minimum required frame rate is 60 frames per second, capturing 6 frames per bit.
To achieve error correction, **H** should record the blink pattern 3 times and use the "majority" for each bit.
The total duration is about 15 seconds.

### LCD Display Interaction

Some devices have 7-segment LED or dot matrix LCD displays of various sizes.
Generally, a 7-segment LED tube can display 2, 4, 6, or 8 decimal digits, while an LCD can display 8, 16, or 32 characters.

In this case, **D** generates a random string *PW* within the range it can display.
For a dot matrix LCD, the generated characters should be limited to `A-Z` and `2-7`, to avoid ambiguity between similar characters.
The user enters this string into **H**.

With this method, 4 decimal digits yield 13 bits of entropy.
8 decimal digits yield 26 bits of entropy.
8 characters yield 40 bits of entropy.

### Additional Considerations

When **D** has better user interaction capabilities, other methods are possible and can provide more entropy in *PW*.

Packet exchange can assist in the timing of OOB password entry, but should not leak any information about *PW*.
For example, in the single light interaction example, **D** can send an Interest to signal the start of a blink pattern, instead of blinking the 50ms-50ms synchronization sequence.
However, it is unacceptable to send the digest of *PW* over the network, even if it is encrypted by a key obtained over the network.
