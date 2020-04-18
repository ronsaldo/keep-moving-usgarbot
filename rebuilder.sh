#!/bin/bash

command()
{
    make -C build-native -j8
}

while CHANGES=$(inotifywait -e close_write src); do
    command
done
