import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';

let renderer;
let scene;
let camera;
let mesh;
let geometry;
let controls;
let raycaster;
let pointer;
let meshes = [];
let layerMap = new Map(); // path string -> mesh
let highlightedMesh = null;
let highlightedLayerPath = null;
let pickHandler = null;
let stageUpAxis = 'Z';
let stageMetersPerUnit = 1.0;

function createRenderer() {
  const canvas = document.createElement('canvas');
  document.body.appendChild(canvas);

  renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
  renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.setPixelRatio(window.devicePixelRatio);
}

function createScene() {
  scene = new THREE.Scene();
  scene.background = new THREE.Color(0xffffff);

  camera = new THREE.PerspectiveCamera(
    45,
    window.innerWidth / window.innerHeight,
    0.1,
    100
  );
  camera.position.set(1.5, 1.2, 1.6);
  camera.up.set(0, 0, 1); // Z-up to match stage
  camera.lookAt(0, 0, 0);

  controls = new OrbitControls(camera, renderer.domElement);
  controls.enableDamping = true;
  controls.dampingFactor = 0.08;
  controls.enablePan = true;
  controls.enableZoom = true;
  controls.target.set(0, 0, 0);

  const ambient = new THREE.AmbientLight(0xffffff, 0.6);
  const dir = new THREE.DirectionalLight(0xffffff, 0.8);
  dir.position.set(2, 3, 4);
  scene.add(ambient, dir);

  geometry = new THREE.BufferGeometry();

  const material = new THREE.MeshStandardMaterial({
    color: 0xffffff,
    metalness: 0.15,
    roughness: 0.45,
    flatShading: true,
    side: THREE.DoubleSide,
  });

  mesh = new THREE.Mesh(geometry, material);
  mesh.frustumCulled = false; // ensure it renders even if bounds are off
  mesh.userData.path = '/World/TestMesh';
  scene.add(mesh);
  meshes = [mesh];
  layerMap.set('/World/TestMesh', mesh);

  raycaster = new THREE.Raycaster();
  pointer = new THREE.Vector2();

  window.addEventListener('pointermove', onPointerMove);
  window.addEventListener('click', onClick);
}

function animate() {
  requestAnimationFrame(animate);
  if (controls) controls.update();
  renderer.render(scene, camera);
}

function handleResize() {
  if (!renderer || !camera) return;
  renderer.setSize(window.innerWidth, window.innerHeight);
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
}

export function initRenderer() {
  createRenderer();
  createScene();
  window.addEventListener('resize', handleResize);
  animate();
}

export function updateMesh(path, vertices) {
  if (!geometry) return;

  // Apply units: scale geometry into meters if needed.
  const scale = stageMetersPerUnit !== 0 ? stageMetersPerUnit : 1.0;
  const scaled = new Float32Array(vertices.length);
  for (let i = 0; i < vertices.length; ++i) {
    scaled[i] = vertices[i] * scale;
  }

  geometry.setAttribute('position', new THREE.BufferAttribute(scaled, 3));
  geometry.setIndex(null); // non-indexed triangle soup
  geometry.setDrawRange(0, vertices.length / 3);
  geometry.computeVertexNormals();
  geometry.computeBoundingSphere();
  geometry.getAttribute('position').needsUpdate = true;

  mesh.userData.path = path;
  layerMap.set(path, mesh);
}

function clearHighlight() {
  if (highlightedMesh) {
    highlightedMesh.material.emissive?.setHex(0x000000);
    highlightedMesh = null;
  }
  if (highlightedLayerPath !== null) {
    const el = document.querySelector(`[data-layer-path="${highlightedLayerPath}"]`);
    if (el) el.classList.remove('selected-layer');
    highlightedLayerPath = null;
  }
}

function highlightMesh(target) {
  clearHighlight();
  highlightedMesh = target;
  highlightedLayerPath = target?.userData?.path ?? null;
  if (target?.material?.emissive) {
    target.material.emissive.setHex(0x3366ff);
  }
  if (highlightedLayerPath !== null) {
    const el = document.querySelector(`[data-layer-path="${highlightedLayerPath}"]`);
    if (el) el.classList.add('selected-layer');
  }
}

function onPointerMove(event) {
  if (!renderer || !camera) return;
  const rect = renderer.domElement.getBoundingClientRect();
  pointer.x = ((event.clientX - rect.left) / rect.width) * 2 - 1;
  pointer.y = -((event.clientY - rect.top) / rect.height) * 2 + 1;
}

function onClick() {
  if (!raycaster || !camera || !renderer) return;
  raycaster.setFromCamera(pointer, camera);
  const hits = raycaster.intersectObjects(meshes, false);
  if (hits.length > 0) {
    highlightMesh(hits[0].object);
    if (pickHandler) pickHandler(hits[0].object.userData.path);
  } else {
    clearHighlight();
  }
}

export function highlightLayer(path) {
  const target = layerMap.get(path);
  if (target) {
    highlightMesh(target);
  } else {
    clearHighlight();
  }
}

export function setPickHandler(cb) {
  pickHandler = cb;
}

export function setStageMetadata(upAxis, metersPerUnit) {
  stageUpAxis = upAxis || 'Z';
  stageMetersPerUnit = metersPerUnit || 1.0;
  if (stageUpAxis === 'Y') {
    camera.up.set(0, 1, 0);
  } else {
    camera.up.set(0, 0, 1);
  }
  controls && controls.update();
}
