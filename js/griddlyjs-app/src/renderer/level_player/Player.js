import { React, Component } from "react";
import Phaser from "phaser";

import HumanPlayerScene from "./scenes/HumanPlayerScene";
import LoadingScene from "../LoadingScene";

class Player extends Component {
  updateCanvasSize = () => {
    this.game.scale.resize(this.props.width, this.props.height);
  };

  componentDidMount() {
    const config = {
      type: Phaser.AUTO,
      parent: this.divElement,
      backgroundColor: "#000000",
      scale: {
        expandParent: false,
      },
      scene: [LoadingScene, HumanPlayerScene],
    };

    this.game = new Phaser.Game(config);

    this.updateCanvasSize();
  }

  componentDidUpdate(prevProps) {
    this.updateCanvasSize();

    if (this.props.griddlyjs) {
      if (prevProps.gdyHash === 0 && this.props.gdy ) {
        this.game.scene.remove("LoadingScene");
        this.game.scene.start("HumanPlayerScene", {
          gdy: this.props.gdy,
          rendererConfig: this.props.rendererConfig,
          rendererName: this.props.rendererName,
          griddlyjs: this.props.griddlyjs,
          getTrajectory: () => this.props.trajectory,
          onDisplayMessage: this.props.onDisplayMessage,
          onTrajectoryComplete: this.props.onTrajectoryComplete,
        });
      } else if (prevProps.gdyHash !== this.props.gdyHash) {
        this.game.scene.getScene("HumanPlayerScene").scene.restart({
          gdy: this.props.gdy,
          rendererConfig: this.props.rendererConfig,
          rendererName: this.props.rendererName,
          griddlyjs: this.props.griddlyjs,
          getTrajectory: () => this.props.trajectory,
          onDisplayMessage: this.props.onDisplayMessage,
          onTrajectoryComplete: this.props.onTrajectoryComplete,
        });
      }
    }
  }

  render() {
    return (
      <div
        ref={(divElement) => {
          this.divElement = divElement;
        }}
      ></div>
    );
  }
}

export default Player;
