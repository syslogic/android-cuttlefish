# Dockerfile based on `rockylinux/rockylinux:9.5`
ARG LINUX_IMAGE=rockylinux/rockylinux:9.5
FROM $LINUX_IMAGE AS base_image
LABEL description="RHEL Integration"
LABEL version="1.0.1"
LABEL repository="https://github.com/google/android-cuttlefish"
LABEL maintainer="Martin Zeitler"
SHELL [ "/bin/bash", "-c" ]

# Don't ask for the `sudo` password.
RUN [ "echo", "%docker", "ALL=(ALL)", "NOPASSWD:", "ALL", ">", "/etc/sudoers.d/docker.conf" ]

# Adding the user provides the home directory.
RUN [ "groupadd", "-g", "117", "docker" ]
RUN [ "useradd", "-g", "117", "-u", "1001", "-s", "/bin/bash", "runner" ]

# Note: The container scripts are checking for this file,
# in order to prevent accidentlly running them on the host.
RUN [ "touch", "/home/runner/.dockerenv" ]

# The EPEL repository installs `dnf-core-plugin` (`config-manager` and `builddeps`) and provides `ncurses-compat-libs`.
# Configuring further package repositories: / https://wiki.rockylinux.org/rocky/repo/#extra-repositories
RUN [ "dnf", "-y", "install", "https://dl.fedoraproject.org/pub/epel/epel-release-latest-9.noarch.rpm" ]
RUN [ "rm", "/etc/yum.repos.d/epel-cisco-openh264.repo" ]
RUN [ "dnf", "config-manager", "--set-enabled", "crb" ]
RUN [ "dnf", "-y", "upgrade" ]
RUN [ "dnf", "clean", "all" ]

# https://docs.docker.com/reference/dockerfile/#notes-about-specifying-volumes
# The host directory is declared at container run-time, with `--volume`, eg.
# --volume /home/username/android-cuttlefish/.rpms/:/home/github/.rpms
RUN [ "mkdir", "/home/runner/.rpms" ]
VOLUME [ "/home/runner/.rpms" ]
WORKDIR "/home/runner"

# Integration
FROM base_image AS integration

# Provide an interactive shell.
ENTRYPOINT [ "/bin/bash" ]

# Expose Operator Port (HTTP:1080, HTTPS:1443)
EXPOSE 1080 1443
# Expose HO(Host Orchestrator) Port (HTTP:2080, HTTPS:2443)
EXPOSE 2080 2443
# Expose WebRTC
EXPOSE 15550-15560
# Expose ADB
EXPOSE 6520-6620

# Import Google RPM repository public key.
# https://www.google.com/linuxrepositories/
RUN [ "dnf", "-y", "install", "wget"]
RUN [ "wget", "https://dl.google.com/linux/linux_signing_key.pub" ]
RUN [ "rpm", "--import", "linux_signing_key.pub" ]

# Add prebuilt RPM packages and install them.
COPY [ "./.rpms/cuttlefish-base-*", "/home/runner/.rpms/" ]
COPY [ "./.rpms/cuttlefish-integration-*", "/home/runner/.rpms/" ]
COPY [ "./.rpms/cuttlefish-user-*", "/home/runner/.rpms/" ]

RUN for file in /home/runner/.rpms/* ; do dnf -y install $file; done
RUN [ "dnf", "clean", "all" ]


# Run commands as user `runner` by default,
# who happens to be the owner of the RPM packages.
