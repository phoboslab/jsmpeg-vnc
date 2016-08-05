// Set the body class to show/hide certain elements on mobile/desktop
document.body.className = ('ontouchstart' in window) ? 'mobile' : 'desktop';


// Setup the WebSocket connection and start the player
var client = new WebSocket( 'ws://' + window.location.host + '/ws' );

var canvas = document.getElementById('videoCanvas');
var player = new jsmpeg(client, {canvas:canvas});


// Input

var mouseLock = !!document.location.href.match('mouselock');
var lastMouse = {x: 0, y: 0};
if( mouseLock ) {
	// FUCK YEAH, VENDOR PREFIXES. LOVE EM!
	canvas.requestPointerLock = canvas.requestPointerLock ||
		canvas.mozRequestPointerLock ||
		canvas.webkitRequestPointerLock ||
		(function(){});
}

// enum input_type_t
var INPUT_KEY = 0x0001,
	INPUT_MOUSE_BUTTON = 0x0002,
	INPUT_MOUSE_ABSOLUTE = 0x0004,
	INPUT_MOUSE_RELATIVE = 0x0008;

var KEY_DOWN = 0x01,
	KEY_UP = 0x00,
	MOUSE_1_DOWN = 0x0002,
	MOUSE_1_UP = 0x0004,
	MOUSE_2_DOWN = 0x0008,
	MOUSE_2_UP = 0x0010;
	MOUSEEVENTF_WHEEL = 0x0800;

// struct input_key_t { uint16 type, uint16 state; uint16 key_code; }
var sendKey = function(ev, action, key) {
	client.send(new Uint16Array([INPUT_KEY, action, key]));
	ev.preventDefault();
};

// struct input_mouse_t { uint16 type, uint16 flags; float32 x; float32 y; int16 amount }
var mouseDataBuffer = new ArrayBuffer(16);
var mouseDataTypeFlags = new Uint16Array(mouseDataBuffer, 0);
var mouseDataCoords = new Float32Array(mouseDataBuffer, 4);
var mouseScrollAmount = new Int32Array(mouseDataBuffer, 12);

var sendMouse = function(ev, action) {
	var type = 0;
	var x, y;

	if( action ) {
		type |= INPUT_MOUSE_BUTTON;

		// Attempt to lock pointer at mouse1 down
		if( mouseLock && action === MOUSE_1_DOWN ) {
			canvas.requestPointerLock();
		}
	}

	// Only make relative mouse movements if no button is pressed
	if( !action && mouseLock ) {
		type |= INPUT_MOUSE_RELATIVE;

		var p = ev.changedTouches ? ev.changedTouches[0] : ev;

		// FUCK, DID I MENTION I LOOOOOVE VENDOR PREFIXES? SO USEFUL!
		x = p.movementX || p.mozMovementX || p.webkitMovementX;
		y = p.movementY || p.mozMovementY || p.webkitMovementY;

		if( typeof x === 'undefined' ) {
			x = p.clientX - lastMouse.x;
			y = p.clientY - lastMouse.y;
		}

		lastMouse.x = p.clientX;
		lastMouse.y = p.clientY;
	}

	// If we send absoulte mouse coords, we can always do so, even for
	// button presses.
	if( !mouseLock ) {
		type |= INPUT_MOUSE_ABSOLUTE;

		var rect = canvas.getBoundingClientRect();
		var scaleX = canvas.width / (rect.right-rect.left),
			scaleY = canvas.height / (rect.bottom-rect.top);

		var p = event.changedTouches ? ev.changedTouches[0] : ev;
		var x = (p.clientX - rect.left) * scaleX,
			y = (p.clientY - rect.top) * scaleY;
	}

	mouseDataTypeFlags[0] = type;
	mouseDataTypeFlags[1] = (action||0);
	mouseDataCoords[0] = x;
	mouseDataCoords[1] = y;
	mouseScrollAmount[0] = (ev.wheelDelta||0);

	client.send(mouseDataBuffer);
	ev.preventDefault();
};


// Keyboard
window.addEventListener('keydown', function(ev) { sendKey(ev, KEY_DOWN, ev.keyCode); }, false );
window.addEventListener('keyup', function(ev) { sendKey(ev, KEY_UP, ev.keyCode); }, false );

// Mouse
canvas.addEventListener('mousemove', function(ev){ sendMouse(ev, null); }, false);
canvas.addEventListener('mousedown', function(ev){ sendMouse(ev, ev.button == 2 ? MOUSE_2_DOWN : MOUSE_1_DOWN); }, false);
canvas.addEventListener('mouseup', function(ev){ sendMouse(ev, ev.button == 2 ? MOUSE_2_UP : MOUSE_1_UP); }, false);
canvas.addEventListener('mousewheel', function(ev){ sendMouse(ev, MOUSEEVENTF_WHEEL); }, false);

// Touch
canvas.addEventListener('touchstart', function(ev){
	lastMouse.x = ev.changedTouches[0].clientX;
	lastMouse.y = ev.changedTouches[0].clientY;
	sendMouse(ev, MOUSE_1_DOWN);
}, false);
canvas.addEventListener('touchend', function(ev){ sendMouse(ev, MOUSE_1_UP); }, false);
canvas.addEventListener('touchmove', function(ev){ sendMouse(ev, null); }, false);

// Touch buttons emulating keyboard keys
var defineTouchButton = function( element, keyCode ) {
	element.addEventListener('touchstart', function(ev){ sendKey(ev, KEY_DOWN, keyCode); }, false);
	element.addEventListener('touchend', function(ev){ sendKey(ev, KEY_UP, keyCode); }, false);
};

var touchKeys = document.querySelectorAll('.key');
for( var i = 0; i < touchKeys.length; i++ ) {
	defineTouchButton(touchKeys[i], touchKeys[i].dataset.code);
}
