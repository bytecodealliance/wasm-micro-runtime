pipeline {
  agent {
    docker {
      image 'node:16'
    }

  }
  stages {
    stage('install dependencies') {
      agent any
      steps {
        sh 'npm install'
      }
    }

    stage('build cli') {
      steps {
        sh 'npm run build'
        sh 'node build/cli/ts2wasm.js tests/benchmark/mandelbrot.ts'
        sh 'node build/cli/ts2wasm.js tests/benchmark/spectral-norm.ts'
      }
    }

    stage('sample test') {
      steps {
        sh 'npm run cover'
        publishHTML (target: [
          allowMissing: false,
          alwaysLinkToLastBuild: false,
          keepAll: true,
          reportDir: '.test_output/coverage',
          reportFiles: 'index.html',
          reportName: "ts2wasm Coverage Report"
        ])
      }
    }

    stage('playground') {
      steps {
        sh '''cd tools/playground/portal
npm install
npm run build'''
        sh '''cd tools/playground/server
npm install
npm run build'''
      }
    }

  }
}