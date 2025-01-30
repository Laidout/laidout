#!/bin/sh

flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user -y flathub org.freedesktop.Platform//20.08
flatpak install --user -y flathub org.freedesktop.Sdk//20.08
