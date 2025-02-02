import gym
import griddly
import numpy as np

env = gym.make("GDY-Sokoban-v0")
env.reset()

available_actions = env.game.get_available_actions(1)
print(available_actions)
player_pos = list(available_actions)[0]
actions = env.game.get_available_action_ids(
    player_pos, list(available_actions[player_pos])
)
print(actions)

clone_env = env.clone()
clone_available_actions = clone_env.game.get_available_actions(1)
print(clone_available_actions)
clone_player_pos = list(clone_available_actions)[0]
clone_actions = clone_env.game.get_available_action_ids(
    clone_player_pos, list(clone_available_actions[clone_player_pos])
)
print(clone_actions)


# Create a bunch of steps and test in both environments
actions = [env.action_space.sample() for _ in range(10000)]

for action in actions:
    obs, reward, done, info = env.step(action)
    c_obs, c_reward, c_done, c_info = clone_env.step(action)

    assert np.all(obs == c_obs)
    assert reward == c_reward
    assert done == c_done
    assert info == c_info

    env_state = env.get_state()
    cloned_state = clone_env.get_state()

    assert env_state["Hash"] == cloned_state["Hash"]

    if done and c_done:
        env.reset()
        clone_env.reset()
