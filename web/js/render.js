import * as THREE from 'https://cdn.jsdelivr.net/npm/three@0.160.0/build/three.module.js';

let renderer;
let scene;
let camera;
let mesh;
let geometry;

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
  camera.lookAt(0, 0, 0);

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
  scene.add(mesh);
}

function animate() {
  requestAnimationFrame(animate);
  if (mesh) mesh.rotation.y += 0.0015;
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

export function updateMesh(vertices) {
  if (!geometry) return;

  geometry.setAttribute(
    'position',
    new THREE.BufferAttribute(vertices, 3)
  );
  geometry.setIndex(null); // non-indexed triangle soup
  geometry.setDrawRange(0, vertices.length / 3);
  geometry.computeVertexNormals();
  geometry.computeBoundingSphere();
  geometry.getAttribute('position').needsUpdate = true;
}
