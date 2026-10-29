# Shared by the scripts in ci/provision/. Sourced, not run.
#
# Each provision script installs one vendor toolchain into the prefix it is
# given and leaves a stamp file there that names what was built (pins and
# build flags). A prefix whose stamp equals what the script would build now
# is complete: a restored cache is used as it is. Any other prefix is built
# again from scratch.
#
# Portability: bash, and the coreutils subset that GNU and uutils agree on
# (the GitHub runner ships uutils). No `mkdir` locks, no `sha256sum -c`.

PROVISION_STAMP=.provision-stamp
PROVISION_MARK=.provision-incomplete

log() { printf '\n== %s\n' "$*"; }
die() { printf 'provision: %s\n' "$*" >&2; exit 1; }

# apt packages, non-interactive. Called on every run, a restored prefix
# included: the vendor libraries are linked against shared libraries from
# these packages, and a fresh runner does not have all of them.
apt_install() {
    sudo env DEBIAN_FRONTEND=noninteractive apt-get update -q
    sudo env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends "$@"
}

# A clone at a tag is only as good as the commit it resolved to: a moved tag
# fails here rather than building something else under the same name.
verify_commit() { # dir commit
    local got
    got="$(git -C "$1" rev-parse HEAD)"
    [ "$got" = "$2" ] || die "commit mismatch in $1: $got, expected $2"
}

# 0 when PREFIX carries exactly STAMP and every named file exists.
prefix_complete() { # prefix stamp file...
    local p="$1" want="$2" f
    shift 2
    [ -f "$p/$PROVISION_STAMP" ] || return 1
    [ "$(cat "$p/$PROVISION_STAMP")" = "$want" ] || return 1
    for f in "$@"; do
        [ -e "$f" ] || return 1
    done
    return 0
}

# Empties PREFIX for a fresh build. The directory itself stays, so a prefix
# created beforehand with the right owner (under /opt, say) stays writable
# without sudo. A non-empty directory without a stamp or an incomplete-mark
# was not made by these scripts and is left alone: a wrong argument should
# stop the run, not delete someone's files.
reset_prefix() { # prefix
    local p="$1"
    if [ -d "$p" ] && [ ! -L "$p" ]; then
        if [ -f "$p/$PROVISION_STAMP" ] || [ -f "$p/$PROVISION_MARK" ] ||
           [ -z "$(ls -A "$p")" ]; then
            find "$p" -mindepth 1 -maxdepth 1 -exec rm -rf {} +
        else
            die "$p exists and was not made by ci/provision; refusing to delete it"
        fi
    elif [ -e "$p" ] || [ -L "$p" ]; then
        die "$p exists and is not a plain directory"
    else
        mkdir -p "$p" || die "cannot create $p"
    fi
}

# The mark makes an interrupted build recognisable as ours on the next run.
mark_prefix() { : > "$1/$PROVISION_MARK"; }

seal_prefix() { # prefix stamp
    printf '%s\n' "$2" > "$1/$PROVISION_STAMP"
    rm -f "$1/$PROVISION_MARK"
}

# Fails when the dynamic loader cannot resolve a dependency of a file. The
# output is captured first: `ldd | grep -q` under pipefail can report the
# SIGPIPE of ldd instead of the match.
check_ldd() { # file...
    local f out
    for f in "$@"; do
        [ -e "$f" ] || die "$f is missing"
        out="$(ldd "$f" 2>&1)" || die "ldd failed on $f: $out"
        case "$out" in
            *"not found"*)
                printf '%s\n' "$out" | grep 'not found' >&2
                die "$f has unresolved shared libraries" ;;
        esac
    done
}

# For the workflow: printed always, appended to $GITHUB_ENV / $GITHUB_PATH
# when the step runs under GitHub Actions.
export_env() { # name value
    printf '  %s=%s\n' "$1" "$2"
    if [ -n "${GITHUB_ENV:-}" ]; then
        printf '%s=%s\n' "$1" "$2" >> "$GITHUB_ENV"
    fi
}
export_path() { # dir
    printf '  PATH += %s\n' "$1"
    if [ -n "${GITHUB_PATH:-}" ]; then
        printf '%s\n' "$1" >> "$GITHUB_PATH"
    fi
}

# A scratch directory for sources and build trees, removed on exit.
make_work() { # name
    WORK="$(mktemp -d "${RUNNER_TEMP:-${TMPDIR:-/tmp}}/provision-$1.XXXXXX")"
    trap 'rm -rf "$WORK"' EXIT
}
