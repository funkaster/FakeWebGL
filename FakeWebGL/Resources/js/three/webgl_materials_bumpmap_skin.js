var statsEnabled = true;

var container, stats, loader;

var camera, scene, renderer;

var mesh, mesh2;

var directionalLight, directionalLight2, pointLight, ambientLight, spotLight;

var mouseX = 0;
var mouseY = 0;

var targetX = 0, targetY = 0;


var windowHalfX = window.innerWidth / 2;
var windowHalfY = window.innerHeight / 2;

var mapColor, mapHeight, mapSpecular;

var firstPass = true;

var composer, composerBeckmann;

init();
animate();

function init() {
	
	container = document.createElement( 'div' );
	document.body.appendChild( container );
	
	//
	
	camera = new THREE.PerspectiveCamera( 27, window.innerWidth / window.innerHeight, 1, 10000 );
	camera.position.z = 1200;
	
	scene = new THREE.Scene();
	
	// LIGHTS
	
	ambientLight = new THREE.AmbientLight( 0x555555 );
	scene.add( ambientLight );
	
	//
	
	pointLight = new THREE.PointLight( 0xffffff, 1.5, 1000 );
	pointLight.position.set( 0, 0, 600 );
	
	scene.add( pointLight );
	
	// shadow for PointLight
	
	spotLight = new THREE.SpotLight( 0xffffff, 1 );
	spotLight.position.set( 0.05, 0.05, 1 );
	scene.add( spotLight );
	
	spotLight.position.multiplyScalar( 700 );
	
	spotLight.castShadow = true;
	spotLight.onlyShadow = true;
	//spotLight.shadowCameraVisible = true;
	
	spotLight.shadowMapWidth = 2048;
	spotLight.shadowMapHeight = 2048;
	
	spotLight.shadowCameraNear = 200;
	spotLight.shadowCameraFar = 1500;
	
	spotLight.shadowCameraFov = 40;
	
	spotLight.shadowBias = -0.005;
	spotLight.shadowDarkness = 0.15;
	
	//
	
	directionalLight = new THREE.DirectionalLight( 0xffffff, 0.85 );
	directionalLight.position.set( 1, -0.5, 1 );
	directionalLight.color.setHSV( 0.6, 0.3, 1 );
	scene.add( directionalLight );
	
	directionalLight.position.multiplyScalar( 500 );
	
	directionalLight.castShadow = true;
	//directionalLight.shadowCameraVisible = true;
	
	directionalLight.shadowMapWidth = 2048;
	directionalLight.shadowMapHeight = 2048;
	
	directionalLight.shadowCameraNear = 200;
	directionalLight.shadowCameraFar = 1500;
	
	directionalLight.shadowCameraLeft = -500;
	directionalLight.shadowCameraRight = 500;
	directionalLight.shadowCameraTop = 500;
	directionalLight.shadowCameraBottom = -500;
	
	directionalLight.shadowBias = -0.005;
	directionalLight.shadowDarkness = 0.15;
	
	//
	
	directionalLight2 = new THREE.DirectionalLight( 0xffffff, 0.85 );
	directionalLight2.position.set( 1, -0.5, -1 );
	scene.add( directionalLight2 );
	
	//
	
	loader = new THREE.JSONLoader( true );
	document.body.appendChild( loader.statusDomElement );
	
	loader.load( "obj/leeperrysmith/LeePerrySmith.js", function( geometry ) { createScene( geometry, 100 ) } );
	
	//
	
	renderer = new THREE.WebGLRenderer( { canvas: document.createElement("canvas"), antialias: false, clearColor: 0x060708, clearAlpha: 1, alpha: false } );
	renderer.setSize( window.innerWidth, window.innerHeight );
//	container.appendChild( renderer.domElement );
	
	var color = new THREE.Color();
	color.setHSV( 0.6, 0.15, 0.35 );
	renderer.setClearColor( color, 1 );
	
	renderer.shadowMapEnabled = true;
	renderer.shadowMapCullFace = THREE.CullFaceBack;
	
	renderer.autoClear = false;
	
	//
	
	renderer.gammaInput = true;
	renderer.gammaOutput = true;
	renderer.physicallyBasedShading = true;
	
	//
	
	if ( statsEnabled ) {
		
		stats = new Stats();
//		container.appendChild( stats.domElement );
		
	}
	
	
	// COMPOSER
	
	renderer.autoClear = false;
	
	// BECKMANN
	
	var effectBeckmann = new THREE.ShaderPass( THREE.ShaderSkin[ "beckmann" ] );
	var effectCopy = new THREE.ShaderPass( THREE.CopyShader );
	
	effectCopy.renderToScreen = true;
	
	var pars = { minFilter: THREE.LinearFilter, magFilter: THREE.LinearFilter, format: THREE.RGBFormat, stencilBufer: false };
	var rtwidth = 512, rtheight = 512;
	
	composerBeckmann = new THREE.EffectComposer( renderer, new THREE.WebGLRenderTarget( rtwidth, rtheight, pars ) );
	composerBeckmann.addPass( effectBeckmann );
	composerBeckmann.addPass( effectCopy );
	
	// EVENTS
	
	document.addEventListener( 'mousemove', onDocumentMouseMove, false );
	window.addEventListener( 'resize', onWindowResize, false );
	
}

function createScene( geometry, scale ) {
	
	var mapHeight = THREE.ImageUtils.loadTexture( "obj/leeperrysmith/Infinite-Level_02_Disp_NoSmoothUV-4096.jpg" );
	
	mapHeight.anisotropy = 4;
	mapHeight.repeat.set( 0.998, 0.998 );
	mapHeight.offset.set( 0.001, 0.001 )
	mapHeight.wrapS = mapHeight.wrapT = THREE.RepeatWrapping;
	mapHeight.format = THREE.RGBFormat;
	
	var mapSpecular = THREE.ImageUtils.loadTexture( "obj/leeperrysmith/Map-SPEC.jpg" );
	
	mapSpecular.anisotropy = 4;
	mapSpecular.repeat.set( 0.998, 0.998 );
	mapSpecular.offset.set( 0.001, 0.001 )
	mapSpecular.wrapS = mapSpecular.wrapT = THREE.RepeatWrapping;
	mapSpecular.format = THREE.RGBFormat;
	
	var mapColor = THREE.ImageUtils.loadTexture( "obj/leeperrysmith/Map-COL.jpg" );
	
	mapColor.anisotropy = 4;
	mapColor.repeat.set( 0.998, 0.998 );
	mapColor.offset.set( 0.001, 0.001 )
	mapColor.wrapS = mapColor.wrapT = THREE.RepeatWrapping;
	mapColor.format = THREE.RGBFormat;
	
	var shader = THREE.ShaderSkin[ "skinSimple" ];
	
	var fragmentShader = shader.fragmentShader;
	var vertexShader = shader.vertexShader;
	
	var uniforms = THREE.UniformsUtils.clone( shader.uniforms );
	
	uniforms[ "enableBump" ].value = true;
	uniforms[ "enableSpecular" ].value = true;
	
	uniforms[ "tBeckmann" ].value = composerBeckmann.renderTarget1;
	uniforms[ "tDiffuse" ].value = mapColor;
	
	uniforms[ "bumpMap" ].value = mapHeight;
	uniforms[ "specularMap" ].value = mapSpecular;
	
	uniforms[ "uAmbientColor" ].value.setHex( 0xa0a0a0 );
	uniforms[ "uDiffuseColor" ].value.setHex( 0xa0a0a0 );
	uniforms[ "uSpecularColor" ].value.setHex( 0xa0a0a0 );
	
	uniforms[ "uRoughness" ].value = 0.145;
	uniforms[ "uSpecularBrightness" ].value = 0.75;
	
	uniforms[ "bumpScale" ].value = 16;
	
	uniforms[ "offsetRepeat" ].value.set( 0.001, 0.001, 0.998, 0.998 );
	
	var material = new THREE.ShaderMaterial( { fragmentShader: fragmentShader, vertexShader: vertexShader, uniforms: uniforms, lights: true } );
	
	mesh = new THREE.Mesh( geometry, material );
	
	mesh.position.y = - 50;
	mesh.scale.set( scale, scale, scale );
	
	mesh.castShadow = true;
	mesh.receiveShadow = true;
	
	scene.add( mesh );
	
	loader.statusDomElement.style.display = "none";
	
}

//

function onWindowResize( event ) {
	
	SCREEN_WIDTH = window.innerWidth;
	SCREEN_HEIGHT = window.innerHeight;
	
	renderer.setSize( SCREEN_WIDTH, SCREEN_HEIGHT );
	
	camera.aspect = SCREEN_WIDTH / SCREEN_HEIGHT;
	camera.updateProjectionMatrix();
	
	
}

function onDocumentMouseMove( event ) {
	
	mouseX = ( event.clientX - windowHalfX ) * 1;
	mouseY = ( event.clientY - windowHalfY ) * 1;
	
}

//

function animate() {
	
	requestAnimationFrame( animate );
	
	render();
	if ( statsEnabled ) stats.update();
	
}

function render() {
	
	targetX = mouseX * .001;
	targetY = mouseY * .001;
	
	if ( mesh ) {
		
		mesh.rotation.y += 0.05 * ( targetX - mesh.rotation.y );
		mesh.rotation.x += 0.05 * ( targetY - mesh.rotation.x );
		
	}
	
	if ( firstPass ) {
		
		composerBeckmann.render();
		firstPass = false;
		
	}
	
	renderer.clear();
	renderer.render( scene, camera );
	
}
