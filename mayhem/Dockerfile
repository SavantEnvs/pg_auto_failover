# syntax=docker/dockerfile:1.7
#
# Commit image for pg_auto_failover: the org C/C++ base plus the Postgres client development
# packages that src/bin/common needs, the PostgreSQL 17 server (and postgresql-common's
# pg_virtualenv) that upstream's monitor regression suite runs against, then the build and test.sh.
FROM ghcr.io/savantenvs/base:20261023

ARG COMMIT
ARG REPO_URL
ARG SOURCE_DATE_EPOCH
ARG SANITIZER_FLAGS
ARG COVERAGE_FLAGS=
ARG MAYHEM_JOBS=

USER root
RUN [ -n "$SOURCE_DATE_EPOCH" ] && repro-sources-list.sh || true ; \
    apt-get update && apt-get install -yq --no-install-recommends \
        libpq-dev postgresql-server-dev-17 libncurses-dev libpam0g-dev libssl-dev \
        libreadline-dev libxml2-dev libxslt1-dev libicu-dev libkrb5-dev libselinux1-dev \
        libzstd-dev liblz4-dev zlib1g-dev \
        postgresql-17 postgresql-common \
 && rm -rf /var/lib/apt/lists/* \
 && (pg_dropcluster --stop 17 main 2>/dev/null || true) \
 && chown -R 2000:2000 "$(pg_config --pkglibdir)" "$(pg_config --sharedir)/extension"
# build.sh runs `make -C src/monitor install` as mayhem (also on the offline PATCH-tier re-run), so
# mayhem owns the extension's install directories.

USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

RUN if [ -x /opt/mayhem/ccache/prefill ]; then /opt/mayhem/ccache/prefill bash mayhem/build.sh; \
    else bash mayhem/build.sh; fi

RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh — repo ships no functional tests"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem
