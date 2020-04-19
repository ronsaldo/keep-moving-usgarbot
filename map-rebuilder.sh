#!/bin/bash

command()
{
    ./pharo Pharo.image eval "KIAGTMapConverter convertAll"
}

while CHANGES=$(inotifywait -e close_write src-assets); do
    command
done
