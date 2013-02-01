function setupGame() {
	chesterGL.settings['useGoogleAnalytics'] = false;
	chesterGL.setup("webgl");
	var size = chesterGL.viewportSize();
	var oneDeg = Math.PI / 180.0;

	chesterGL.loadAsset("texture", "images/test.png");
	chesterGL.loadAsset("texture", "images/star.png");
	chesterGL.assetsLoaded("texture", function () {
		// $("#loading").html("Test Single Block");
		// finish with the setup and run the game
		chesterGL.setupPerspective();

		var sceneBlock = new chesterGL.Block(null, chesterGL.Block.TYPE['SCENE']);
		sceneBlock.title = "Test::Single Block";
		chesterGL.setRunningScene(sceneBlock);

		// create a block
		var someBlock = new chesterGL.Block();
		someBlock.setTexture("images/test.png");
		// someBlock.rotateBy(-45);
		someBlock.setPosition([size.width/2, size.height/2, 0]);

		var someBlock2 = new chesterGL.Block();
		someBlock2.setTexture("images/star.png");
		someBlock2.setColor([1, 1, 1, 0.5]);
		someBlock2.setPosition([60, 0, 0]);
		someBlock.addChild(someBlock2);

		var pt = someBlock.getAbsolutePosition([size.width/2, size.height/2, 0]);
		console.log("point: " + pt[0] + "," + pt[1]);

		sceneBlock.addChild(someBlock);

		// add some action
		var dz = 10;
		someBlock.setUpdate(function () {
			this.setRotation(this.rotation + oneDeg);
			// this.setPosition([this.position[0], this.position[1], this.position[2] + dz]);
			// if (this.position[2] >=  200) { dz = -dz; }
			// if (this.position[2] <= -200) { dz = -dz; }
		});
		someBlock2.setUpdate(function () {
			this.setRotation(this.rotation - oneDeg);
		});

		chesterGL.run();
		// draw a single frame (for debug purposes)
		// chesterGL.drawScene();
	});
}
