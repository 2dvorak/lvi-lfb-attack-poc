# PoC for the LVI-LFB Control Flow Hijacking attack ([CVE-2020-0551](https://nvd.nist.gov/vuln/detail/CVE-2020-0551))

This repository holds the sources for the LVI-LFB Control Flow Hijacking attack PoC.

## Contents

* lvi-cfh-poc - hijack the control flow of another process via line-fill buffer spraying
* whitepaper

## Prerequisites

1. nasm, gcc, make
2. A vulnerable Intel CPU

## Tested Environment

CPU            | OS           | Kernel         | GCC version | NASM version | Vulnerable?
---            | ---          | ---            | :---:       | :---:        | :---:
Intel i7-8700K | Ubuntu 18.04 | 5.4.0-generic  | 7.5.0       | 2.13.02      | Yes

## Authors

* Andrei Vlad LUȚAȘ
* Dan Horea LUȚAȘ

## Additional resources

* Bitdefender blog post: https://businessinsights.bitdefender.com/bitdefender-researchers-discover-new-side-channel-attack
* Official LVI web-site - https://lviattack.eu
* Intel Security Advisory - https://www.intel.com/content/www/us/en/security-center/advisory/intel-sa-00334.html
* Intel Deep Dive - https://software.intel.com/security-software-guidance/insights/deep-dive-load-value-injection
