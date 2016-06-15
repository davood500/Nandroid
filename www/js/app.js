(function (app) {
    app.config(["$stateProvider", "$urlRouterProvider", function ($stateProvider, $url) {
        $url.otherwise("/login");
        $stateProvider.state("login", {
            templateUrl: "./views/login.html",
            controller: "loginCtrl"
        }).state("dashboard", {
            url: "/",
            templateUrl: "./views/dashboard.html",
            controller: "dashboardCtrl"
        });
    }]);

    app.service("nandroidService", ["$http", "$q", function ($http, $q) {
        return ({
            getVersion: function () {
                var def = $q.defer();
                $http.get("/api/version")
                    .success(function (data) {
                        def.resolve(data);
                    })
                    .error(function () {
                        def.reject("Failed to get version");
                    });
                return def.promise;
            },
            getGreeting: function(){
                var def = $q.defer();
                $http.get("/api/data/get_greeting")
                    .success(function(data){
                        def.resolve(data);
                    })
                    .error(function(){
                        def.reject("Failed to get greeting");
                    });
                return def.promise;
            },
            setGreeting: function(new_greeting){
                var def = $q.defer();
                $http.post("/api/data/set_greeting", new_greeting)
                    .success(function(data){
                        def.resolve(data);
                    })
                    .error(function(){
                        def.reject("Failed to set greeting");
                    });
                return def.promise;
            }
        });
    }]);

})(angular.module("nandroid", ["ui.router"]));