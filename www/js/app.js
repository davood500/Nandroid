(function(app){
    app.service("nandroidService", ["$http", "$q", function($http, $q){
        return ({
            getVersion: function(){
                var def = $q.defer();
                $http.get("/api/version")
                    .success(function(data){
                        def.resolve(data);
                    })
                    .error(function(){
                        def.reject("Failed to get version");
                    });
                return def.promise;
            }
        });
    }]);

    app.controller("mainCtrl", ["$scope", "nandroidService", function($scope, $nan){
        $nan.getVersion()
            .then(function(data){
                $scope.version = data.version;
            }, function(data){
                $scope.version = data;
            });
    }]);
})(angular.module("nandroid", [ "ui.router" ]));