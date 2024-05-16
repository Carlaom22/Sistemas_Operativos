#!/bin/bash

# Pedro Lima 2017276603
# Carlos Soares 2020230124

randint() {
    echo $(( RANDOM % ($2 - $1 + 1 ) + $1 ))
}

# Get the number of users from the first command-line argument, or default to 10
num_users=${1:-5}

# Limit the number of users to 50
if (( num_users > 50 )); then
    num_users=50
fi

# Create 10 mobile users
for i in $(seq 1 $num_users)
do
    start_plafond=$(randint 100 1000)
    max_auth_requests=$(randint 5 40)
    video_interval=$(randint 2000 8000)
    music_interval=$(randint 2000 8000)
    social_interval=$(randint 2000 8000)
    data_reserve=$(randint 30 100)

    ./mobile_user $start_plafond $max_auth_requests $video_interval $music_interval $social_interval $data_reserve &
done