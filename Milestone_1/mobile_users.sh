#!/bin/bash

# Pedro Lima 2017276603
# Carlos Soares 2020230124

randint() {
    echo $(( RANDOM % ($2 - $1 + 1 ) + $1 ))
}

# Create 10 mobile users
for i in {1..10}
do
    start_plafond=$(randint 10 50)
    max_auth_requests=$(randint 20 30)
    video_interval=$(randint 2 10)
    music_interval=$(randint 2 10)
    social_interval=$(randint 2 10)
    data_reserve=$(randint 1 10)

    ./exe/mobile_user $start_plafond $max_auth_requests $video_interval $music_interval $social_interval $data_reserve &
done